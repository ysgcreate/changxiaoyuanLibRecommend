/*************************************************************************
	> File Name: UserIIF.cpp
	> Author: ysg
	> Mail: ysgcreate@gmail.com
	> Created Time: 2015年11月29日 星期日 11时59分11秒
 ************************************************************************/
#include "global.h"
#include "cJSON.h"
using namespace std;
#define random(x) (rand()%x)
/*读缓冲区的大小*/
#define READ_BUFFER_SIZE 128
/*写缓冲区的大小*/
#define WRITE_BUFFER_SIZE 2048
#ifndef MapCmpByStringDouble
#define MapCmpByStringDouble
struct CmpByValue {
  bool operator()(const pair<string,double> & lhs, const pair<string,double> & rhs){
      return lhs.second>rhs.second;
  }
};
#endif
class UserIIF {
private:
    int m_sockfd;
    char m_read_buf[READ_BUFFER_SIZE];
    char* m_write_buf;
    double Recall(int N);
    double Popularity(int N);
    double Coverage(int N);
    double Precision(int N);
    void init(){
        memset(m_read_buf,'\0',READ_BUFFER_SIZE);
    };
public:
    UserIIF (){}
    ~UserIIF (){}
    static vector<pair<string,string>> data;
    static map<string,vector<string>> test;
    static map<string,vector<string>> train;
    static map<string,map<string,double>> W;
    static map<string,vector<pair<string,double>>> ranks;
    static int m_epollfd;
    static int K;
    void init(int sockfd,bool);
    void close();
    bool readfrom();
    bool writeto();
    void process();
    vector<pair<string,double>> Recommend(string user,int N);
    void testRes(int N);
};
void UserIIF::testRes(int N){
    printf("召回率:%.2lf ",Recall(N));
    printf("准确率:%.2lf ",Precision(N));
    printf("覆盖率:%.2lf ",Coverage(N));
    printf("新颖度:%.2lf\n",Popularity(N));
}
void UserIIF::init(int sockfd,bool debug) {//debug
    m_sockfd=sockfd;
    // /*避免TIME_WAIT状态,仅用于调试,实际使用时应该去掉*/
    if (debug) {
        int reuse=1;
        setsockopt(m_sockfd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse));//一个端口释放两分钟后才能使用,reuseaddr不用等两分钟
    }
    init();
    addfd(m_epollfd,sockfd,true);
}
void UserIIF::close() {
    removefd(m_epollfd,m_sockfd);
}
bool UserIIF::readfrom(){
    memset(m_read_buf,'\0',READ_BUFFER_SIZE);
    int bytes_read=0;
    while (true) {
        bytes_read=recv(m_sockfd,m_read_buf,READ_BUFFER_SIZE,0);
        if (bytes_read<0) {
            if (errno==EAGAIN||errno==EWOULDBLOCK) {
                break;
            }
            return false;
        }else if (bytes_read==0) {
            return false;
        }
    }
    return true;
}
bool UserIIF::writeto(){
    int temp=0;
    int bytes_have_send=0;
    int bytes_to_send=strlen(m_write_buf);
    if (bytes_to_send==0) {
        modfd(m_epollfd,m_sockfd,EPOLLIN);
        init();
        return true;
    }
    while (1) {
        temp=send(m_sockfd,m_write_buf+bytes_have_send,sizeof(m_write_buf),0);
        if (temp<=-1) {
            /*如果TCP写缓冲没有空间,则等待下一轮的EPOLLOUT 事件,虽然在此期间,服务器无法立即接受到统一客户的下一个请求,但这可以保证连接的完整性*/
            if (errno==EAGAIN) {
                modfd(m_epollfd,m_sockfd,EPOLLOUT);
            }
            return false;
        }
        bytes_to_send-=temp;
        bytes_have_send+=temp;
        if (bytes_to_send<=0) {
            /*发送响应成功*/
            // printf("end %d %d\n",bytes_to_send,bytes_have_send);
            init();
            modfd(m_epollfd,m_sockfd,EPOLLIN);
            return true;
        }
    }
}
void UserIIF::process(){
    string tmp;
    for(int i=0;i<sizeof(m_read_buf);i++){
        if(m_read_buf[i]=='\n'||m_read_buf[i]=='\0'||m_read_buf[i]=='\r'){
            break;
        }
        tmp.insert(tmp.end(),1,m_read_buf[i]);
    }
    auto recommens=Recommend(tmp,10);
    cJSON *root,*fmt;
    root=cJSON_CreateObject();
    cJSON* pArray = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "items", pArray);
    if (recommens.size()>0) {
        cJSON_AddItemToObject(root,"code",cJSON_CreateNumber(1));
        for (auto item:recommens) {
            cJSON_AddItemToArray(pArray, cJSON_CreateString(item.first.c_str()));
        }
    }else{
        cJSON_AddItemToObject(root,"code",cJSON_CreateNumber(-1));
    }
    char* p = cJSON_Print(root);
    m_write_buf=p;
    cJSON_Delete(root);
    modfd(m_epollfd,m_sockfd,EPOLLOUT);
}
vector<pair<string,double>> UserIIF::Recommend(string user,int N){
    if(ranks.find(user)!=ranks.end()){
        return ranks[user];
    }
	map<string,double> rank;
	vector<string> interacted_items=train[user];
	vector<pair<string,double>> userRank;
	for (auto it=W[user].begin();it!=W[user].end();it++) {
        userRank.push_back(make_pair(it->first,it->second));
    }
    sort(userRank.begin(),userRank.end(),CmpByValue());
	int rvi=1;
	for(int i=0;i<K&&i<userRank.size();i++){
		auto v=userRank[i].first;
		double wuv=userRank[i].second;
		for(auto item :train[v]){
			int flag=0;
			for(int j=0;j<interacted_items.size();j++){
				if(item==interacted_items[j]){
					flag=1;
					break;
				}
			}
			if (flag) {
				continue;
			}
			if (rank.find(item)!=rank.end()) {
				rank[item]+=1.0*wuv*rvi;
			}else{
				rank[item]=1.0*wuv*rvi;
			}
		}
	}
	vector<pair<string,double>> rankv;
	for (auto it=rank.begin();it!=rank.end();it++) {
		rankv.push_back(make_pair(it->first,it->second));
        // cout<<it->first<<" "<<it->second<<endl;
	}
	sort(rankv.begin(),rankv.end(),CmpByValue());
	// for(int i=0;i<rankv.size()&&i<100;i++){
	// 	cout<<rankv[i].first<<" "<<rankv[i].second<<endl;
	// }
	if(N<rankv.size()){
		vector<pair<string,double>> v;
		v.assign(rankv.begin(),rankv.begin()+N);
        ranks[user]=v;
		return v;
	}else{
		return rankv;
	}
}
double UserIIF::Recall(int N){
	double hit=0;
	double all=0;
	for(auto user_items:test){
		auto user=user_items.first;
		auto items=user_items.second;
		auto rank=Recommend(user,N);
		for(auto itemone:items){
			for(auto itemtwo:rank){
				if(itemone==itemtwo.first){
					hit+=1;
				}
			}
		}
		all+=items.size();
	}
	return hit/all;
}
double UserIIF::Precision(int N){
	double hit=0;
	double all=0;
	for(auto user_items:test){
		auto user=user_items.first;
		auto items=user_items.second;
		auto rank=Recommend(user,N);
		for(auto itemone:items){
			for(auto itemtwo:rank){
				if(itemone==itemtwo.first){
					hit+=1;
				}
			}
		}
		all+=rank.size();
	}
	return hit/all;
}
double UserIIF::Coverage(int N){
	map<string,int> recommned_items;
	map<string,int> all_items;
	for(auto user_items:train){
		auto user=user_items.first;
		auto items=user_items.second;
	    for(auto item:items){
	        if(all_items.find(item)==all_items.end()){
				all_items[item]=1;
			}
		}
	    auto rank=Recommend(user,N);
	    for(auto item:rank){
			if(recommned_items.find(item.first)==recommned_items.end()){
				recommned_items[item.first]=1;
			}
		}
	}
	double res=recommned_items.size();
	double als=all_items.size();
	return res/als;
}
double UserIIF::Popularity(int N){
	map<string,int> item_popularity;
	for(auto user_items : train){
		auto user=user_items.first;
		auto items=user_items.second;
	    for(auto item : items){
	        if(item_popularity.find(item)==item_popularity.end()){
	            item_popularity[item] = 0;
			}
	        item_popularity[item] += 1;
		}
	}
    double ret = 0;
    double n = 0;
	for(auto user_items : train){
		auto user=user_items.first;
		auto items=user_items.second;
		auto rank = Recommend(user,N);
        for(auto item:rank){
            ret += log(1+ item_popularity[item.first]);
            n += 1;
		}
	}
    return ret/n;
}
bool readData(char* file) {
	FILE *fp;
	char buf[48];
	if(!(fp=fopen(file,"r")))
		return false;
	while (fgets(buf,sizeof(buf),fp)!=NULL) {
        char delims[] = "::";
        char *result = NULL;
        result = strtok( buf, delims );
        string tmp1=result;
        result = strtok( NULL, delims );
        string tmp2=result;
		UserIIF::data.push_back(make_pair(tmp1,tmp2));
	}
	fclose(fp);
	return true;
}
void SplitData(int M,int k){
	int testCnt,trainCnt;
	testCnt=0;
	trainCnt=0;
	for (size_t i = 0; i < UserIIF::data.size(); i++) {
		if (random(M)==k) {
			UserIIF::test[UserIIF::data[i].first].push_back(UserIIF::data[i].second);
			testCnt++;
		}else{
			UserIIF::train[UserIIF::data[i].first].push_back(UserIIF::data[i].second);
			trainCnt++;
		}
	}
}
void UserSimilarity(){
	//build inverse table for item_users
	map<string,vector<string>> item_users;
	for(auto it :UserIIF::train){
		string userid=it.first;
		for (auto item: it.second) {
			item_users[item].push_back(userid);
		}
	}
	//calculate co-rated items between users;
	map<string,map<string,double>> C;
	map<string,double> N;
	for(auto item_user:item_users){
		auto item=item_user.first;
		auto users=item_user.second;
		for(auto u:users){
			if(N.find(u)!=N.end()){
				N[u]+=1;
			}else{
				N[u]=1;
			}
			for(auto v:users){
				if(u==v)
					continue;
				if(C.find(u)==C.end()||C[u].find(v)==C[u].end()){
					C[u][v]=1/log(1+users.size());
				}else{
					C[u][v]+=1/log(1+users.size());
				}
			}
		}
	}
	//calculate final similarity matrix W
	for(auto users:C){
		auto u=users.first;
		for(auto vusers:users.second){
			string v=vusers.first;
			double cuv=vusers.second;
			UserIIF::W[u][v]=cuv/sqrt(N[u]*N[v]);
			//printf("%d %d %d %d %d %lf %lf\n",u,v,cuv,N[u],N[v],sqrt(N[u]*N[v]),W[u][v]);
		}
	}
}
vector<pair<string,string>> UserIIF::data;
map<string,vector<string>> UserIIF::test;
map<string,vector<string>> UserIIF::train;
map<string,map<string,double>> UserIIF::W;
map<string,vector<pair<string,double>>> UserIIF::ranks;
int UserIIF::K=20;
int UserIIF::m_epollfd;
