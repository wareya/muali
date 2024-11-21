#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>

#include "types.hpp"

#include "grammar.hpp"
#include "compiler.hpp"

int main(int argc, char ** argv)
{
    (void)argc;
    (void)argv;
    
    auto f = fopen("src_c/grammar.txt", "rb");
    assert(f);
    Vec<char> text;
    int c;
    while ((c = fgetc(f)) >= 0)
        text.push_back(c);
    text.push_back(0);
    
    auto f2 = fopen("test.mua", "rb");
    assert(f2);
    Vec<char> text2;
    while ((c = fgetc(f2)) >= 0)
        text2.push_back(c);
    text2.push_back(0);
    
    auto grammar = load_grammar(text.data());
    
    puts("HW!");
    
    debug_print_grammar_points(grammar);
    
    auto tokens = tokenize(grammar, text2.data());
    
    if (tokens.size() == 0)
    {
        puts("Error: program is empty.");
        return 0;
    }
    if (tokens.back()->text == 0)
    {
        print_tokenization_error(tokens, String(text2.data()));
        puts("failed to tokenize");
        return 0;
    }
    
    size_t i = 0;
    for (auto n : tokens)
    {
        if (n->from_regex)
            printf("> %zd\t%s (via %s)\n", i, n->text->data(), n->from_regex->str.data());
        else
            printf("> %zd\t%s\n", i, n->text->data());
        i += 1;
    }
    
    auto asdf = parse_as(grammar, tokens, "program");
    
    if (!asdf)
    {
        print_parse_error(tokens, String(text2.data()));
        puts("failed to parse");
        return 0;
    }
    
    return 0;
}
