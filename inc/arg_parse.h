#pragma once

#include "clipp.h"
#include <iostream>
#include <sstream>

struct ArgParser{

    std::string output_path = "";
    std::string input_path = "../test/test.sd";
    bool show_warning = true;
    bool print_ast = false;
    bool print_ast_sym = false;
    bool no_color = false;

    std::string ir_target = "spl";

    clipp::group get_parser(){
        using namespace clipp;
        return (
            opt_value("input", input_path),

            option("-v", "--version").call([](){
                std::cout << "stardust version 0.1.0" << std::endl;
                exit(0);
            }) % "version info",

            option("-o", "--output")&value("output path", output_path)
                .if_missing([](){std::cout << "require output path after option -o." << std::endl;})
                .if_repeated([](){std::cout << "repeat option -o." << std::endl;}),

            option("-q", "--quiet").set(show_warning, false).doc("suppress all warnings")
                .if_repeated([](){std::cout << "repeat option -q." << std::endl;}),

            option("--print-ast").set(print_ast, true) |
            option("--no-print-ast").set(print_ast, false).if_conflicted([](){
                std::cout << "conflict setting of --print-ast" << std::endl;
            }),

            option("--print-sym").set(print_ast_sym, true) |
            option("--no-print-sym").set(print_ast_sym, false).if_conflicted([](){
                std::cout << "conflict setting of --print-sym" << std::endl;
            }),

            option("-h", "--help").call([this](){
                std::cout << clipp::make_man_page(this->get_parser(), "stardust") << std::endl;
                exit(0);
            }) % "usage lines",

            option("--ir=spl").set(ir_target, std::string("spl")) |
            option("--ir=llvm").set(ir_target, std::string("llvm")).if_conflicted([](){
                std::cout << "conflict setting of --ir target" << std::endl;
            }) % "set intermediate representation format",
            option("-nc", "--no-color").set(no_color, true)
        );
    }

    void parse(int argc, char** argv){
        auto parser = get_parser();
        auto res = clipp::parse(argc, argv, parser);
        if(!res){
            std::cout<< clipp::make_man_page(parser, "stardust") << std::endl;
            exit(0);
        }
    }

    std::string man_page(){
        std::stringstream ss;
        ss << clipp::make_man_page(get_parser(), "stardust");
        return ss.str();
    }
};