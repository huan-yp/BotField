//Chinese UTF-8
#include <iostream>
#include <vector>
#include <iterator>
#include <climits>
#include <ctime>
#include <algorithm>
#include <sstream>
#include <cstdio>
#include <set>
#include <random>
#include <omp.h>
#include <chrono>
#include <ctime>
#include <cstdlib>
#include <iomanip>
#include <cstring>
#include <filesystem>
#include "jsoncpp/json.h"
#include "yaml_parser.h"
#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;
namespace fs = std::filesystem;

// 并行
#define PARALLEL

// 全局配置变量
int TOTAL_GAMES = 20;
int PLAYER_NUMBER = 12;
string BOT_DIR = "bots";
string DEFAULT_BOT = "demo";
vector<pair<string, string>> bots;

struct node {
    vector<int> score, round;
    void init(int size) {
        score.resize(size, 0);
        round.resize(size, 0);
    }
};
map<string,node> Final_score;
map<string,int> player_winning_rounds, player_winning_scores;
std::mt19937_64 rng(time(0));

constexpr short card2level(short card){return card/4 + card/53;}

template <class T>
void print_vector(vector<T> vec)
{
	copy(vec.begin(), vec.end(), ostream_iterator<T>(cout, " "));
	cout << endl;
}

template <class T>
void print_set(set<T> set)
{
	for (T c : set)
	{
		int j = card2level(c) + 3;
		if (j == 14)j = 1;
		else if (j == 15)j = 2;
		else if (j == 16)j = 0;
		else if (j == 17)j = -1;
		std::cout << j << " ";
	}
	cout << endl;
}

string get_response(const char* cmd)
{
	char result[2048] = "";
	char buf_ps[2048];
	char ps[2048] = {};
	FILE* ptr;

	strcpy(ps, cmd);  // 如编译不通过的话将strcpy_s改成strcpy
#ifdef _WIN32
	for (int i = 0; ps[i] != 0; ++i) {
		if (ps[i] == '/') {
			ps[i] = '\\';
		}
	}
#endif
	if ((ptr = _popen(ps, "r")) != NULL)
	{
		while (fgets(buf_ps, 2048, ptr) != NULL)strcat_s(result, buf_ps);
		_pclose(ptr);
		ptr = NULL;
	}
	else cout << "_popen " << ps << " error" << endl;
	string ret(result);
	return ret;
}

const vector<string> colors = {
    "\033[31m", // 红
    "\033[32m", // 绿
    "\033[33m", // 黄
    "\033[34m", // 蓝
    "\033[35m", // 品红
    "\033[36m"  // 青
};

// 加载配置并初始化 bots
bool load_config() {
    SimpleYamlParser config;
    
    // 尝试从当前目录加载 config.yaml
    if (!config.parse("config.yaml") && !config.parse("config.yaml")) {
        cerr << "Error: Cannot load config.yaml" << endl;
        return false;
    }

    TOTAL_GAMES = config.getInt("total_games", 20);
    PLAYER_NUMBER = config.getInt("player_number", 12);
    BOT_DIR = config.getString("bot_dir", "bots");
    DEFAULT_BOT = config.getString("default_bot", "demo");

    // 扫描 bot 目录
    vector<string> bot_files;
    string bot_path = BOT_DIR;
    
    try {
        if (fs::exists(bot_path) && fs::is_directory(bot_path)) {
            for (const auto& entry : fs::directory_iterator(bot_path)) {
                if (entry.is_regular_file() && entry.path().extension() == ".exe") {
                    bot_files.push_back(entry.path().filename().string());
                }
            }
        }
    } catch (const exception& e) {
        cerr << "Error scanning bot directory: " << e.what() << endl;
        return false;
    }

    // 如果找不到 bot，使用默认 bot
    if (bot_files.empty()) {
        cerr << "Warning: No bots found in " << bot_path << ", using default bot" << endl;
        for (int i = 0; i < PLAYER_NUMBER; i++) {
            bots.push_back({DEFAULT_BOT, "Player" + to_string(i + 1)});
        }
        return true;
    }

    // 填充 bots 数组，使用完整路径
	string default_bot_path = BOT_DIR + "/" + DEFAULT_BOT;
    bots.clear();
    for (size_t i = 0; i < bot_files.size() && bots.size() < (size_t)PLAYER_NUMBER; i++) {
        string bot_name = bot_files[i];
        string player_name = bot_name.substr(0, bot_name.find_last_of('.'));
        string full_path = BOT_DIR + "/" + bot_name;
		string path_without_suffix = full_path.substr(0, full_path.find_last_of('.'));
		// cout<<"Debug: "<<path_without_suffix<<' '<<default_bot_path<<"\n";
		if(path_without_suffix == default_bot_path) continue;
        bots.push_back({full_path, player_name});
    }
	
    // 如果不足 PLAYER_NUMBER 个，用 DEFAULT_BOT 补全
    int current_count = bots.size();
	// cout<<"Size:"<<current_count<<'\n';

    for (int i = current_count; i < PLAYER_NUMBER; i++) {
        bots.push_back({default_bot_path, "Default" + to_string(i - current_count + 1)});
    }

    return true;
}

void print_init()
{
    stringstream ss;
    ss << "JSON_DATA:{\"type\":\"init\",\"total_games\":" << TOTAL_GAMES 
       << ",\"player_number\":" << PLAYER_NUMBER << ",\"players\":[";

    for(int i=0; i<PLAYER_NUMBER; i++)
    {
        if(i > 0) ss << ","; // 输出玩家的名称和对应的exe文件名
        ss << "{\"name\":\"" << bots[i].second << "\",\"exe\":\"" << bots[i].first << "\"}";
    }
    ss << "]}";
    
    // 立即输出并刷新缓冲区
    cout << ss.str() << endl;
}

// 替换原有的 print_rank 函数
void print_rank(int game_num)
{
    // 1. 排序
    sort(bots.begin(), bots.end(), [](const pair<string, string>& a, const pair<string, string>& b) {
        return player_winning_scores[a.second] > player_winning_scores[b.second];
    });

    // 2. 构建 JSON 字符串
    // 手动构建 JSON 字符串以避免依赖外部库的复杂性，确保格式为 JSON_DATA:{...}
    stringstream ss;
    ss << "JSON_DATA:{\"type\":\"rank_update\",\"game_num\":" << game_num 
       << ",\"total_games\":" << TOTAL_GAMES << ",\"data\":[";

    for (int i = 0; i < PLAYER_NUMBER; i++)
    {
        auto [exe_name, name] = bots[i];
        if (i > 0) ss << ",";
        ss << "{\"rank\":" << (i + 1)
           << ",\"name\":\"" << name << "\""
           << ",\"exe\":\"" << exe_name << "\""
           << ",\"score\":" << player_winning_scores[name]
           << ",\"wins\":" << player_winning_rounds[name] << "}";
    }
    ss << "]}";

    // 3. 输出并刷新缓冲区 (非常重要)
    cout << ss.str() << endl;
}

int main()
{
	#ifdef _WIN32
    SetConsoleOutputCP(65001); 
    #endif
    // 加载配置
    if (!load_config()) {
        cerr << "Failed to load configuration" << endl;
        return 1;
    }

    // 初始化 Final_score
    for (const auto& bot : bots) {
        Final_score[bot.first].init(TOTAL_GAMES);
        player_winning_rounds[bot.second] = 0;
        player_winning_scores[bot.second] = 0;
    }

    // cout << colors[0] << "(test)" << "\033[0m" << '\n';
	// freopen("result.txt","w",stdout);
	print_init();
	auto start = std::chrono::high_resolution_clock::now();
	for(int game_no = 0; game_no<TOTAL_GAMES; game_no++) 
	{
		vector<short> cards;
		set<short> player_initial_cards[3];
		set<short> public_cards;
		for(short card=0; card<54; card++)
            cards.push_back(card);
		shuffle(cards.begin(), cards.end(), rng);
        shuffle(bots.begin(), bots.end(), rng);

        for (int i=0; i<3; i++)
        {
            player_initial_cards[i].clear();
			player_initial_cards[i].insert(cards.begin()+i*17, cards.begin()+(i+1)*17);
        }
		public_cards.clear();
		public_cards.insert(cards.begin()+51, cards.begin()+54);

#ifdef PARALLEL
        #pragma omp parallel for
#endif
        for(int now=0; now<PLAYER_NUMBER; now+=3) //bot [now, now+1, now+2] in a battle
        {
            int score = 1;
            int landlord_position;
            set<short> player_cards[3];
            vector<string> requests[3];
		    vector<string> responses[3];
			for(int i=0; i<3; i++) requests[i].clear();
			for(int i=0; i<3; i++) responses[i].clear();
			for(int i=0; i<3; i++) player_cards[i] = player_initial_cards[i];
			int player_bid[3];
			int final_bid;
			string requests0, responses0, cmd0;
			requests0 += "{\"\"\"own\"\"\":[";
			requests0 += to_string(*player_cards[0].begin());
			for (set<short>::iterator i = next(player_cards[0].begin()); i != player_cards[0].end(); i++)
			{
				requests0 += ",";
				requests0 += to_string(*i);
			}
			requests0 += "],\"\"\"bid\"\"\":[]}";
			responses0 = "";
			requests[0].push_back(requests0);
			cmd0 = bots[now].first + " \"{\"\"requests\"\"\":[" + requests0 + "],\"\"\"responses\"\"\":[" + responses0 + "]}\"";
			string response0;
			response0 = get_response(cmd0.c_str());
			
			Json::Reader reader;
			Json::Value input0;
			reader.parse(response0, input0);
			player_bid[0] = input0["response"].asInt();
			responses[0].push_back(to_string(player_bid[0]));
			// cout << "player_bid[" << bots_no[sub_game][0] << "]: " << player_bid[0] << endl;
			string requests1, responses1, cmd1;
			requests1 += "{\"\"\"own\"\"\":[";
			requests1 += to_string(*player_cards[1].begin());
			for (set<short>::iterator i = next(player_cards[1].begin()); i != player_cards[1].end(); i++)
			{
				requests1 += ",";
				requests1 += to_string(*i);
			}
			requests1 += "],\"\"\"bid\"\"\":[";
			requests1 += to_string(player_bid[0]);
			requests1 += "]}";
			responses1 = "";
			requests[1].push_back(requests1);
			cmd1 = bots[now+1].first + " \"{\"\"requests\"\"\":[" + requests1 + "],\"\"\"responses\"\"\":[" + responses1 + "]}\"";
			//cout << "cmd1: " << cmd1 << endl;
			string response1;
			response1 = get_response(cmd1.c_str());
			
			Json::Value input1;
			reader.parse(response1, input1);
			player_bid[1] = input1["response"].asInt();
			responses[1].push_back(to_string(player_bid[1]));
			//cout << "player_bid[" << bots_no[sub_game][1] << "]: " << player_bid[1] << endl;
			string requests2, responses2, cmd2;
			requests2 += "{\"\"\"own\"\"\":[";
			requests2 += to_string(*player_cards[2].begin());
			for (set<short>::iterator i = next(player_cards[2].begin()); i != player_cards[2].end(); i++)
			{
				requests2 += ",";
				requests2 += to_string(*i);
			}
			requests2 += "],\"\"\"bid\"\"\":[";
			requests2 += to_string(player_bid[0]);
			requests2 += ",";
			requests2 += to_string(player_bid[1]);
			requests2 += "]}";
			responses2 = "";
			requests[2].push_back(requests2);
			cmd2 = bots[now+2].first + " \"{\"\"requests\"\"\":[" + requests2 + "],\"\"\"responses\"\"\":[" + responses2 + "]}\"";
			// cout << "cmd2: " << cmd2 << endl;
			string response2;
			response2 = get_response(cmd2.c_str());
			
			// cout << "response2: " << response2 << endl;
			Json::Value input2;
			reader.parse(response2, input2);
			player_bid[2] = input2["response"].asInt();
			responses[2].push_back(to_string(player_bid[2]));
			//cout << "player_bid[" << bots_no[sub_game][2] << "]: " << player_bid[2] << endl;
			landlord_position = 0;
			if (player_bid[1] > player_bid[0])
			{
				if (player_bid[2] > player_bid[1])landlord_position = 2;
				else landlord_position = 1;
			}
			else if (player_bid[2] > player_bid[0])landlord_position = 2;
			final_bid = *max_element(player_bid, player_bid + 3);
			//cout << "final_bid: " << final_bid << endl;
			score = max(final_bid, 1);

			int turn = landlord_position;
			set<short> history[2];  // history[0]为上上家的出牌记录，history[1]为上家的出牌记录
			//First round first
			for (int i = 0; i < 3; i++)
			{
				string tmp_requests, tmp_responses, tmp_cmd, tmp_response;
				tmp_requests += "{\"\"\"history\"\"\":[[";
				if (history[0].size())
				{
					tmp_requests += to_string(*history[0].begin());
					for (set<short>::iterator j = next(history[0].begin()); j != history[0].end(); j++)
					{
						tmp_requests += ",";
						tmp_requests += to_string(*j);
					}
				}
				tmp_requests += "],[";
				if (history[1].size())
				{
					tmp_requests += to_string(*history[1].begin());
					for (set<short>::iterator j = next(history[1].begin()); j != history[1].end(); j++)
					{
						tmp_requests += ",";
						tmp_requests += to_string(*j);
					}
				}
				tmp_requests += "]],\"\"\"own\"\"\":[";
				tmp_requests += to_string(*player_cards[turn].begin());
				for (set<short>::iterator j = next(player_cards[turn].begin()); j != player_cards[turn].end(); j++)
				{
					tmp_requests += ",";
					tmp_requests += to_string(*j);
				}
				tmp_requests += "],\"\"\"publiccard\"\"\":[";
				tmp_requests += to_string(*public_cards.begin());
				for (set<short>::iterator j = next(public_cards.begin()); j != public_cards.end(); j++)
				{
					tmp_requests += ",";
					tmp_requests += to_string(*j);
				}
				tmp_requests += "],\"\"\"landlord\"\"\":";
				tmp_requests += to_string(landlord_position);
				tmp_requests += ",\"\"\"pos\"\"\":";
				tmp_requests += to_string(turn);
				tmp_requests += ",\"\"\"finalbid\"\"\":";
				tmp_requests += to_string(final_bid);
				tmp_requests += "}";
				requests[turn].push_back(tmp_requests);
				tmp_cmd += bots[now+turn].first;
				tmp_cmd += " \"{\"\"requests\"\"\":[";
				tmp_cmd += requests[turn][0];
				tmp_cmd += ",";
				tmp_cmd += requests[turn][1];
				tmp_cmd += "],\"\"\"responses\"\"\":[";
				tmp_cmd += *responses[turn].begin();
				tmp_cmd += "]}\"";

				// cout << "tmp_cmd: " << tmp_cmd << endl;
				tmp_response = get_response(tmp_cmd.c_str());
				// cout << "tmp_response: " << tmp_response << endl;

				Json::Value output;
				Json::Reader reader;
				reader.parse(tmp_response, output);
				history[0] = history[1];
				history[1].clear();
				for (int j = 0; j < output["response"].size(); j++)history[1].insert(output["response"][j].asInt());
				if (i == 0)player_cards[landlord_position].insert(public_cards.begin(), public_cards.end());
				/*cout << bots_no[sub_game][turn] << ": ";
				print_set(history[1]);*/
				for (short card : history[1])player_cards[turn].erase(card);
				if (history[1].size() == 2 && history[1].find(52) != history[1].end() && history[1].find(53) != history[1].end())score *= 2;  // 王炸
				else if (history[1].size() == 4 && *history[1].begin() == *prev(history[1].end()))score *= 2;  // 炸弹
				tmp_responses.clear();
				tmp_responses += "[";
				if (history[1].size())
				{
					tmp_responses += to_string(*history[1].begin());
					for (set<short>::iterator j = next(history[1].begin()); j != history[1].end(); j++)
					{
						tmp_responses += ",";
						tmp_responses += to_string(*j);
					}
				}
				tmp_responses += "]";
				responses[turn].push_back(tmp_responses);
				turn = (turn + 1) % 3;
			}
			//And then next turns ...
			bool landlord_has_not_played = true;
			while (true)
			{
				string tmp_requests, tmp_responses, tmp_cmd, tmp_response;
				tmp_requests += "{\"\"\"history\"\"\":[[";
				if (history[0].size())
				{
					tmp_requests += to_string(*history[0].begin());
					for (set<short>::iterator j = next(history[0].begin()); j != history[0].end(); j++)
					{
						tmp_requests += ",";
						tmp_requests += to_string(*j);
					}
				}
				tmp_requests += "],[";
				if (history[1].size())
				{
					tmp_requests += to_string(*history[1].begin());
					for (set<short>::iterator j = next(history[1].begin()); j != history[1].end(); j++)
					{
						tmp_requests += ",";
						tmp_requests += to_string(*j);
					}
				}
				tmp_requests += "]]}";
				requests[turn].push_back(tmp_requests);
				tmp_cmd += bots[now+turn].first;
				tmp_cmd += " \"{\"\"requests\"\"\":[";
				tmp_cmd += *requests[turn].begin();
				for (vector<string>::iterator i = next(requests[turn].begin()); i != requests[turn].end(); i++)
				{
					tmp_cmd += ",";
					tmp_cmd += *i;
				}
				tmp_cmd += "],\"\"\"responses\"\"\":[";
				tmp_cmd += *responses[turn].begin();
				for (vector<string>::iterator i = next(responses[turn].begin()); i != responses[turn].end(); i++)
				{
					tmp_cmd += ",";
					tmp_cmd += *i;
				}
				tmp_cmd += "]}\"";
				Json::Value output;
				Json::Reader reader;

				// cout << "tmp_cmd: " << tmp_cmd << endl;
				tmp_response = get_response(tmp_cmd.c_str());
				// cout << "tmp_response: " << tmp_response << endl;	
				
				reader.parse(tmp_response, output);
				//cout << "tmp_cmd: " << tmp_cmd << endl;
				history[0] = history[1];
				history[1].clear();
				for (int i = 0; i < output["response"].size(); i++)history[1].insert(output["response"][i].asInt());
				/*cout << bots_no[sub_game][turn] << ": ";
				print_set(history[1]);*/
				for (short card : history[1])player_cards[turn].erase(card);
				if (history[1].size() == 2 && history[1].find(52) != history[1].end() && history[1].find(53) != history[1].end())score *= 2;  // 王炸
				else if (history[1].size() == 4 && *history[1].begin() == *prev(history[1].end()))score *= 2;  // 炸弹
				if (turn == landlord_position && history[1].size())landlord_has_not_played = false;  // 地主除了第一手之外出过牌了
				tmp_responses.clear();
				tmp_responses += "[";
				if (history[1].size())
				{
					tmp_responses += to_string(*history[1].begin());
					for (set<short>::iterator j = next(history[1].begin()); j != history[1].end(); j++)
					{
						tmp_responses += ",";
						tmp_responses += to_string(*j);
					}
				}
				tmp_responses += "]";
				responses[turn].push_back(tmp_responses);

				if (!player_cards[turn].size())
				{
					if (turn == landlord_position)  //landlord win
					{
						if (player_cards[(turn+1)%3].size()==17 && player_cards[(turn+2)%3].size()==17)score*=2;  // 地主春天				
                        // cout << "landlord is " << bots[now+turn].first << '\n';
                        Final_score[bots[now+turn].first].round[game_no] ++;
                        Final_score[bots[now+turn].first].score[game_no] += 2*score;
                        player_winning_rounds[bots[now+turn].second]++;
                        player_winning_scores[bots[now+turn].second]+=2*score;
						for (int i=0; i<3; i++)
							if (i != turn) 
                            {   
                                Final_score[bots[now+i].first].score[game_no] -= score;
                                player_winning_scores[bots[now+i].second] -= score;
                            }
					}
					else  // 农民胜利
					{
						if (landlord_has_not_played) score *= 2;  // 农民春天		
                        Final_score[bots[now+landlord_position].first].score[game_no] -= 2*score;
                        player_winning_scores[bots[now+landlord_position].second] -= 2*score;
						for(int i=0; i<3; i++)
						{
							if(i != landlord_position) Final_score[bots[now+i].first].score[game_no] += score;
							if(i != landlord_position) Final_score[bots[now+i].first].round[game_no]++;
							if(i != landlord_position) player_winning_scores[bots[now+i].second] += score;
							if(i != landlord_position) player_winning_rounds[bots[now+i].second] ++;
						}
					}

#ifdef PARALLEL
					// #pragma omp critical
					// {
					// 	cout << "thread is " << omp_get_thread_num() << ", game " << game_no << " is "; 
					// 	cout << "over, winner: " << bots[now+turn].first;
					// 	if (turn != landlord_position) cout << ", " << bots[now+3-turn-landlord_position].first;
					// 	cout << "; landlord: " << bots[now+landlord_position].first;
					// 	cout << "; score: " << score << endl;
					// }
					// printf("thread id = %d game %d round %d, winner is %d, score is %d\n", omp_get_thread_num(),game_no,sub_game,bots_no[sub_game][turn],score);
#endif
#ifndef PARALLEL
					cout << "game " << game_no << " is "; 
					cout << "over, winner: " << bots[now+turn].first;
					if (turn != landlord_position)cout << ", " << bots[now+3-turn-landlord_position].first;
					cout << "; landlord: " << bots[now+landlord_position].first;
					cout << "; score: " << score << endl;
#endif
					break;
				}
				turn = (turn + 1) % 3;
			}
		}
        print_rank(game_no);
	}
#ifdef PARALLEL
	#pragma omp barrier
	#pragma omp single
#endif
    sort(bots.begin(), bots.end(), [](const pair<string,string>& a, const pair<string,string>& b){
            return player_winning_scores[a.second]>player_winning_scores[b.second];});
	cout << "result: " << '\n';
    for(int player=0; player<PLAYER_NUMBER; player++)
    {
        string name=bots[player].second;
        cout << name << ", win_rounds = " << player_winning_rounds[name] << ", win_scores = " << player_winning_scores[name] << '\n';
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    cout << "Elapsed time: " << duration << " ms\n";
}