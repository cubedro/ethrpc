#include <iostream>
#include <fstream>
#include <string>
#include <curl/curl.h>

using namespace std;

//-------------------------------------------------------------------------
// simple macros
//-------------------------------------------------------------------------
#define quote(a) (string("\"") + a + "\"")
#define max(a,b) ((a>b)?a:b)

//-------------------------------------------------------------------------
extern void curl_init (void);
extern void curl_clean(void);
extern bool getBlock  (string& block, int num);

//-------------------------------------------------------------------------
// Confirm user input, setup 'curl', get the block, the cleanup and quit
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
	{
		string blockStr;
		if (getBlock(blockStr,i))
		{
			cout << blockStr << (i!=stop?",":"") << "\n";
// If you want to store to a file, uncomment this and comment above line
//			string fileName = to_string(i) + ".json";
//			std::ofstream out(fileName);
//			out << blockStr << "\n";
//			out.close();
//			if (!(i%5)) { cout << i << "\r";cout.flush();}
		}
	}
	if (stop-start) cout << "]";

	curl_clean();
	return 0;
}

//-------------------------------------------------------------------------
extern CURL  *curlPtr       ( bool cleanup=false );
extern size_t write_callback( char *ptr, size_t size, size_t nmemb, void *userdata );
extern string callRPC       ( const string& method, const string& params, const string& id );

//-------------------------------------------------------------------------
// Use 'curl' to ask the node for a block.
//-------------------------------------------------------------------------
bool getBlock(string& block, int num)
{
	block = callRPC("eth_getBlockByNumber", "["+quote(to_string(num))+",true]", to_string(num));
	return true;
}

//-------------------------------------------------------------------------
// Use 'curl' to make an arbitrary rpc call
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

