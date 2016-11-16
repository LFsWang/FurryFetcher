#define CURL_STATICLIB
#include<curl/curl.h>

#define CHROME_HEADER R"(Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/54.0.2837.0 Safari/537.36)"
#include<Windows.h>
#include<iostream>
#include<fstream>
#include<string>
#include<regex>

using std::cout;
using std::cerr;
using std::endl;
using std::cin;

//Source http://stackoverflow.com/questions/9786150/save-curl-content-result-into-a-string-in-c
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents,size* nmemb);
    return size * nmemb;
}

bool htmlGetToString(std::string url,std::string &html)
{
    std::string tmp;
    std::string head;

    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, CHROME_HEADER);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &tmp);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);

    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &head);

    curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "cookies.txt");
    curl_easy_setopt(curl, CURLOPT_COOKIEJAR, "cookies.txt");
    //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
    {
        std::cerr << curl_easy_strerror(res) << std::endl;
        return false;
    }
    html = std::move(tmp);
    return true;
}

#define SETFILE "last.txt"
int GetLastCode()
{
    std::ifstream fin(SETFILE);
    if (!fin)return INT_MAX;
    int tmp;
    fin >> tmp;
    if (!fin)return INT_MAX;
    return tmp;
}

void SetLastCode(int code)
{
    std::ofstream fout(SETFILE);
    fout << code;
}

int GetSiteLastCode(std::string html)
{
    std::regex Rule(R"A(PostModeMenu\.click\((\d*)\))A");
    std::smatch res;

    if (!std::regex_search(html, res, Rule))
    {
        return -1;
    }
    return std::stoi(res[1]);
}
bool Exist(int Code)
{
    WIN32_FIND_DATA FindFileData;
    HANDLE handle = FindFirstFile((std::to_string(Code)+".*").c_str(), &FindFileData);
    if (handle != INVALID_HANDLE_VALUE)
    {
        FindClose(handle);
        return true;
    }
    return false;
}

bool getData(std::string site,int Code)
{
    std::string html;
    if (Exist(Code)) {
        cerr << Code << " Exist!" << endl;
        return true;
    }
    if (!htmlGetToString(site + "post/show/" + std::to_string(Code), html)) {
        cerr << Code << " Get HTML Error!" << endl;
        return false;
    }

    std::regex Rule(R"A(Size:.*\/data\/(.*)\" id)A");
    std::smatch res;
    if (!std::regex_search(html, res, Rule))
    {
        cerr << Code << " Not Exist!" << endl;
        return false;
    }

    std::string download = site + "data/" + (std::string)res[1];
    cout << "Download " << Code << ' ' << download << endl;
    if( !htmlGetToString(download,html) )
    {
        cerr << Code << " Download Fail!" << endl;
        return false;
    }
    int sz = (int)download.size();
    std::string fname = std::to_string(Code) + download.substr(sz - 4);

    std::ofstream fout(fname, std::ios::out | std::ios::binary);
    fout.write(html.data(), html.size());
    fout.close();
}

int main()
{
    curl_global_init(CURL_GLOBAL_DEFAULT);

    std::string site;
    std::string data;
    cin >> site;
    if (!htmlGetToString(site+"post/", data))
    {
        cerr << "Cannot cennect site:" << site << endl;
        exit(-1);
    }
    
    //Find intro point
    int last_code = GetLastCode();
    int site_code = GetSiteLastCode(data);

    if (site_code == -1)
    {
        cerr << "Cannot Get SiteLast Code!" << endl;
        exit(-1);
    }
    while (site_code)
    {
        getData(site, site_code--);
        Sleep(1000 * 10);//Prevent DDOS
    }
    return 0;
}