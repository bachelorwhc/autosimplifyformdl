#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iterator>
#include <string>
#include <map>
#include <set>
#include <boost/algorithm/string.hpp>
#include <boost/spirit/home/x3.hpp>
#include <regex>
namespace x3 = boost::spirit::x3;

namespace rules
{
    using namespace x3;

    auto WORD           = ( alnum | char_( '_' ) );
    auto NAMESPACE      = -string("::") >> +WORD >> "::";
    auto CONSTANT       = ( lexeme[ '"' >> *~char_( '"' ) >> '"' ] | +digit >> -char_('.') >> *digit >> -char_( 'f' ) );
    auto ident          = lexeme[ char_( "A-Za-z_" ) >> *WORD ];
    auto VARIABLE       = *NAMESPACE >> ident % '.';
    auto OPERAND        = -char_( "+-" ) >> ( VARIABLE | CONSTANT );
    auto S_EXPRESSION   = -char_( "+-" ) >> OPERAND % char_( "*+/-" );
    auto P_EXPRESSION   = -char_( "+-" ) >> '(' >> S_EXPRESSION >> ')';
    auto EXPRESSION     = 
        ( ( -char_( "+-" ) >> (S_EXPRESSION | P_EXPRESSION) ) % char_( "*+/-" ) ) | 
        ( '?' >> ( S_EXPRESSION | P_EXPRESSION ) >> char_(':') >> ( S_EXPRESSION | P_EXPRESSION ) );
    auto ARGUMENTS      = EXPRESSION % ',';
    auto FUNCTION_CALL  = VARIABLE >> '(' >> -ARGUMENTS >> ')';

    auto simple_function = rule<struct simple_function_, std::string>{ "simple_function" }
    = skip( space )[ x3::raw[ FUNCTION_CALL ] ];
#ifdef _DEBUG
    auto expression = rule<struct simple_function_, std::string>{ "expression" }
    = skip( space )[ x3::raw[ EXPRESSION ] ];
#endif
}

int main( int argc, char** argv )
{
    if ( argc < 3 )
    {
        std::cout << "autosimplifyformdl [filename] [prfix] [export]";
        return 1;
    }
    
    std::string filename( argv[ 1 ] );
    
    std::ifstream file( filename );

    if ( file.is_open() )
    {
        std::string context( std::istreambuf_iterator<char>( file ), {} );
        std::vector<std::string> calls;
        std::vector<std::string> expressions;
        std::map<std::string, int> function_call_times;

        size_t start_pos = 0;
        std::regex start_pattern( "let\\s*\\{" );
        std::smatch start_match;
        if ( std::regex_search( context, start_match, start_pattern ) )
            start_pos = start_match.suffix().first - context.begin();

        std::regex breadcrumb_pattern( "_RL_REDUCT_BREADCRUMB_" );
        std::smatch breadcrumb_match;
        while ( std::regex_search( context.cbegin() + start_pos, context.cend(), breadcrumb_match, breadcrumb_pattern ) )
            start_pos = breadcrumb_match.suffix().first - context.begin();

        std::string target_context( context.begin() + start_pos, context.end() );
        parse( target_context.begin(), target_context.end(), *x3::seek[ rules::simple_function ], calls );
#ifdef _DEBUG
        parse( target_context.begin(), target_context.end(), *x3::seek[ rules::expression], expressions );
#endif
        context.erase( context.begin() + start_pos, context.end() );
        
        if ( calls.size() )
        {
            for ( const auto& call : calls )
            {
                if ( function_call_times.count( call ) < 1 )
                {
                    function_call_times[ call ] = 1;
                }
                else
                {
                    ++function_call_times[ call ];
                }
            }
        }

        const std::map<std::string, std::string> function_return_type = {
            {"((::)?math::cos|::math::sin)\\(.*\\)", "float"},
            {"((::)?state::texture_coordinate)\\(.*\\)", "float3"},
            {"((::)?state::texture_tangent_u)\\(.*\\)", "float3"},
            {"((::)?state::texture_tangent_v)\\(.*\\)", "float3"},
            {"((::)?base::texture_coordinate_info)\\(.*\\)", "::base::texture_coordinate_info"},
            {"((::)?base::anisotropy_conversion)\\(.*\\)", "::base::anisotropy_return"},
            {"((::)?state::normal)\\(.*\\)", "float3"},
            {"((::)?df::diffuse_edf)\\(.*\\)", "edf"},
            {"((::)?math::luminance)\\(.*\\)", "float"},
            {"((::)?base::gloss_to_rough)\\(.*\\)", "float"},
            {"((::)?df::light_profile_maximum)\\(.*\\)", "float"},
            {"((::)?texture_isvalid)\\(.*\\)", "bool"},
            {"((::)?meters_per_scene_unit)\\(.*\\)", "float"},
            {"((::)?base::transform_coordinate)\\(.*\\)", "::base::texture_coordinate_info"},
            {"((::)?base::file_texture)\\(.*\\)", "::base::texture_return"},
            {"((::)?nvidia::core_definitions::blend_colors)\\(.*\\)", "::base::texture_return"},
            {"(int)\\(.*\\)", "int"},
            {"(color)\\(.*\\)", "color"},
            {"(float)\\(.*\\)", "float"},
            {"(float2)\\(.*\\)", "float2"},
            {"(float3)\\(.*\\)", "float3"},
            {"(float4)\\(.*\\)", "float4"},
            {"(float3x3)\\(.*\\)", "float3x3"},
            {"(float4x4)\\(.*\\)", "float4x4"},
            {"(texture_2d)\\(.*\\)", "texture_2d"},
            {".*(edf)\\(.*\\)", "edf"},
            {".*(bsdf)\\(.*\\)", "bsdf"},
        };

        std::regex ignore_pattern(
            "((::)?anno::.*\\(.*\\))|"
            "(contribution\\(.*\\))|"
            "(.*annotations.*\\(.*\\))|"
            "(ui_position\\(.*\\))|"
            "(type_of_material\\(.*\\))|"
            "(typical_object_size\\(.*\\))|"
            "(suitable_as_light\\(.*\\))|"
            "(abs\\(.*\\))|"
            "(log\\(.*\\))"
        );

        bool need_export = argc > 3;

        int serial = 0;
        std::string prefix( argv[ 2 ] );
        for ( const auto& kv : function_call_times )
        {
            if ( kv.second > 1 )
            {
                const auto& call = kv.first;
                
                std::smatch dummy;
                if ( std::regex_match( call, dummy, ignore_pattern ) )
                {
                    continue;
                }
                std::string replace = prefix + std::to_string( serial++ );

                auto reduct_exp_to_value = 
                    [ &target_context, need_export ]
                (const std::string& type, const std::string& name, const std::string& exp) -> void 
                {
                    auto value_exp = type + " " + name + " = " + exp + ";";
                    if ( need_export )
                    {
                        boost::replace_all( target_context, exp, name );
                        target_context = value_exp + "  // _RL_REDUCT_BREADCRUMB_\n" + target_context;
                    }
                    std::cout << value_exp << std::endl;
                };

                bool found = false;
                for ( const auto& pkv : function_return_type )
                {
                    const auto& p = pkv.first;
                    const auto& t = pkv.second;
                    std::regex pattern( p );
                    std::smatch match;
                    if ( std::regex_match( call, match, pattern ) )
                    {
                        found = true;

                        reduct_exp_to_value( t, replace, call );
                        std::cout << call << " -> " << replace << " : " << kv.second << std::endl;
                        break;
                    }
                }
                if ( !found )
                {
                    std::cout << call << " return type UNKNOWN! called" << kv.second << " times." << std::endl;
                }
                std::cout << std::endl;
            }
        }

        file.close();

        if( need_export )
        {
            std::string export_filename( argv[ 3 ] );
            std::ofstream export_file( export_filename );
            if ( export_file.is_open() )
            {
                export_file << context << "\n" << target_context;
                export_file.close();
            }
            else
            {
                std::cout << "failed to write " << export_filename << '!' << std::endl;
            }
        }
    }
    else
    {
        std::cout << "file not opened." << std::endl;
    }
    return 0;
}