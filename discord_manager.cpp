#include <iostream>
#include <string>
#include <cstdlib>
#include <curl/curl.h>

using namespace std;

size_t WriteCallback(void* contents, size_t size, size_t nmemb, string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

string discordRequest(const string& method, const string& url, const string& token, const string& payload = "") {
    CURL* curl;
    CURLcode res;
    string readBuffer;

    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method.c_str());
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        struct curl_slist* headers = NULL;
        string authHeader = "Authorization: Bot " + token;
        headers = curl_slist_append(headers, authHeader.c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "User-Agent: DiscordBot (https://github.com, 1.0)");
        
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        if (!payload.empty()) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
        }

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    return readBuffer;
}

int main(int argc, char** argv) {
    if (argc < 4) {
        cout << "Usage: " << argv[0] << " <token> <channel_id> <mode> [args...]" << endl;
        return 1;
    }

    string token = argv[1];
    string channel_id = argv[2];
    string mode = argv[3];
    string base_url = "https://discord.com/api/v10/channels/" + channel_id + "/messages";

    if (mode == "fetch") {
        string url = base_url + "?limit=1";
        string response = discordRequest("GET", url, token);

        if (response == "[]" || response.empty()) {
            cout << "empty|" << endl;
            return 0;
        }

        // Removed spaces from JSON keys for minified string parsing
        size_t id_pos = response.find("\"id\":\"");
        if (id_pos != string::npos) {
            id_pos += 6;
            string msg_id = response.substr(id_pos, response.find("\"", id_pos) - id_pos);

            string target_link = "";
            size_t attach_pos = response.find("\"attachments\":[");
            size_t url_pos = string::npos;
            
            if (attach_pos != string::npos) {
                size_t end_attach = response.find("]", attach_pos);
                url_pos = response.find("\"url\":\"", attach_pos);
                if (url_pos > end_attach) url_pos = string::npos;
            }

            if (url_pos != string::npos) {
                url_pos += 7;
                target_link = response.substr(url_pos, response.find("\"", url_pos) - url_pos);
            } else {
                size_t content_pos = response.find("\"content\":\"");
                if (content_pos != string::npos) {
                    content_pos += 11;
                    target_link = response.substr(content_pos, response.find("\"", content_pos) - content_pos);
                    size_t escape = target_link.find("\\n");
                    if (escape != string::npos) target_link = target_link.substr(0, escape);
                }
            }
            cout << msg_id << "|" << target_link << endl;
        } else {
            cout << "error|" << response << endl;
        }
    } 
    else if (mode == "delete" && argc >= 5) {
        string msg_id = argv[4];
        string url = base_url + "/" + msg_id;
        discordRequest("DELETE", url, token);
        cout << "Message deleted." << endl;
    }
    else if (mode == "send" && argc >= 5) {
        string status_text = argv[4];
        for(int i = 5; i < argc; ++i) {
            status_text += " ";
            status_text += argv[i];
        }
        string payload = "{\"content\": \"" + status_text + "\"}";
        discordRequest("POST", base_url, token, payload);
        cout << "Message sent." << endl;
    }

    return 0;
}
