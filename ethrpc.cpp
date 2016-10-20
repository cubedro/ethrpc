/*--------------------------------------------------------------------------------
 The MIT License (MIT)
 Copyright (c) 2016 Great Hill Corporation
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 --------------------------------------------------------------------------------*/
#include <iostream>
#include <fstream>
#include <string>
#include <curl/curl.h>

using namespace std;

//-------------------------------------------------------------------------
#define max(a,b) ((a>b)?a:b)
extern void   curl_init          (void);
extern void   curl_clean         (void);
extern string getBlock           (int blockNum);
extern string getTransaction     (const string& hash);
extern string getReceipt         (const string& hash);
extern string getTransactionTrace(const string& hash);

//-------------------------------------------------------------------------
// Confirm user input, setup 'curl', get the block, cleanup, then quit
//-------------------------------------------------------------------------
int main(int argc, const char *argv[])
{
	if (argc<2 || argc>3)
	{
		fprintf(stderr,"\n\tPlease provide a start (and optionally stop) blocknumber.\n\n");
		exit(0);
	}

	curl_init();

	unsigned long start =                strtoul(argv[1], NULL, 10);
	unsigned long stop  = (argc==3 ? max(strtoul(argv[2], NULL, 10), start+1) : start);

	if (stop-start) cout << "[";
	for (int i=start;i<=stop;i++)
		cout << getBlock(i) << (i!=stop?",":"") << "\n";
	if (stop-start) cout << "]";

	/* - see below
	cout << "\n";
	cout << "getTransaction:\n"        << getTransaction     ("0xb229fbd196624dc38a213b62a32974651d826c7aae4646bea1130b7d9adcf560") << "\n\n";
	cout << "getTransactionReceipt:\n" << getReceipt         ("0xb229fbd196624dc38a213b62a32974651d826c7aae4646bea1130b7d9adcf560") << "\n\n";
	cout << "getTransactionTrace:\n"   << getTransactionTrace("0xb229fbd196624dc38a213b62a32974651d826c7aae4646bea1130b7d9adcf560") << "\n\n";
	*/

	curl_clean();

	return 0;
}

//-------------------------------------------------------------------------
extern CURL  *curlPtr       ( bool cleanup=false );
extern size_t write_callback( char *ptr, size_t size, size_t nmemb, void *userdata );
extern string callRPC       ( const string& method, const string& params, const string& id="");
#define quote(a) (string("\"") + a + "\"")

//-------------------------------------------------------------------------
string getBlock(int num)
{
	return callRPC("eth_getBlockByNumber", "["+quote(to_string(num))+",true]", to_string(num));
}

/*
//-------------------------------------------------------------------------
// These functions are not use here, but could easily be used to get
// more information about transcations, etc.
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
void padLeft(string& str, const size_t num, const char paddingChar=' ')
{
    if (num > str.size())
        str.insert(0,num-str.size(),paddingChar);
}
string getTransaction(const string& hash)
{
    // assumes the hash starts with '0x'
    string h = hash.substr(2,hash.length());padLeft(h,64,'0');
    return callRPC("eth_getTransactionByHash", "[" + quote(h) +"]");
}
string getReceipt(const string& hash)
{
    string h = hash.substr(2,hash.length());padLeft(h,64,'0');
    return callRPC("eth_getTransactionReceipt", "[" + quote(h) +"]");
}
string getTransactionTrace(const string& hash)
{
    return "traceTransaction not supported unless you start geth with the 'debug' api interface";
//	string h = hash.substr(2,hash.length());padLeft(h,64,'0');
//	return callRPC("debug_traceTransaction", "[" + quote(h) +"]");
}
*/

//-------------------------------------------------------------------------
string callRPC(const string& method, const string& params, const string& id)
{
	string thePost;
	thePost += "{";
	thePost +=  quote("jsonrpc") + ":"  + quote("2.0")  + ",";
	thePost +=  quote("method")  + ":"  + quote(method) + ",";
	thePost +=  quote("params")  + ":"  + params        + ",";
	thePost +=  quote("id")      + ":"  + quote(id);
	thePost += "}";

	curl_easy_setopt(curlPtr(), CURLOPT_POSTFIELDS,    thePost.c_str());
	curl_easy_setopt(curlPtr(), CURLOPT_POSTFIELDSIZE, thePost.length());

	string result;
	curl_easy_setopt(curlPtr(), CURLOPT_WRITEDATA,     &result);
	curl_easy_setopt(curlPtr(), CURLOPT_WRITEFUNCTION, write_callback);

	CURLcode res = curl_easy_perform(curlPtr());
	if (res != CURLE_OK)
	{
		fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		exit(0);
	}

	// Remove the json result wrapper so we get a clean block
	result.erase(0,1);                // skip over first bracket
	result.erase(0,result.find("{")); // clear to the next bracket
	result.erase(result.end()-1);     // erase the last bracket
	return result;
}

//-------------------------------------------------------------------------
// 'curl' calls this function as each portion of the request is processed.
// As we accumulate the string, we strip newlines (\n) and return (\r)
//-------------------------------------------------------------------------
size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	string tmpStr;
	int cnt=0;
	for (int i=0;i<nmemb;i++)
	{
		if (ptr[i]!='\n' && ptr[i]!='\r')
		{
			tmpStr += ptr[i];
			cnt++;
		}
	}
	tmpStr[cnt] = '\0';

	// store it away in the caller's string
	(*(string*)userdata) += tmpStr;

	// we handeled everything, tell curl to send more if there is any
	return size*nmemb;
}

//-------------------------------------------------------------------------
CURL *curlPtr(bool cleanup)
{
	static CURL *curl = NULL;
	static struct curl_slist *headers = NULL;
	if (!curl)
	{
		curl = curl_easy_init();
		if (!curl)
		{
			fprintf(stderr, "Curl failed to initialize. Quitting...\n");
			exit(0);
		}
		headers = curl_slist_append(headers,       "Content-Type: application/json");
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_URL,        "http:/""/localhost:8545"); // 'geth' or 'parity' must be serving RPC here

	} else if (cleanup)
	{
		if ( headers ) curl_slist_free_all(headers);
		if ( curl    ) curl_easy_cleanup  (curl);
		curl = NULL;
	}

	return curl;
}

//-------------------------------------------------------------------------
void curl_init(void)
{
	curlPtr();
}

//-------------------------------------------------------------------------
void curl_clean(void)
{
	curlPtr(true);
}
