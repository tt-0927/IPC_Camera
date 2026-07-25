#pragma once
#include "map"
#include "vector"
#include "string"
#include "fstream"
#include "iostream"

class TokenizerClip
{
private:
    std::map<std::string, int> tokenizer_token2idx;

private:
    std::vector<std::string> stringSplit(const std::string &str, char delim);
    void tokenize(std::string token, std::vector<int> &idx);

public:
    TokenizerClip(std::string sVocabPath);
    bool load_tokenize(std::string vocab_path);
    bool encode_text(std::string text, std::vector<int> &idx);
};
class TokenizerClipChinese
{
private:
    std::vector<std::string> stringSplit(const std::string &str, char delim);

    std::map<std::string, int> tokenizer_token2idx;
    int CLS = 101;
    int SEP = 102;

public:
    TokenizerClipChinese(std::string sVocabPath);
    bool load_tokenize(std::string vocab_path);
    bool encode_text(std::string text, std::vector<int> &idx);
};
