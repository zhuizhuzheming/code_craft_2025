#include<math.h>
#include<iostream>
#include<vector>
#include<queue>
#include<string>
#include<map>
#include<unordered_map>
#include<algorithm>
#include<future>
#include<thread>
#include<mutex>
#include<set>
//负载均衡的问题
// using namespace std;
//std::ios::sync_with_stdio(false);
//std::cin.tie(nullptr);
//control:beginusestrategy=14400/controldelay=10/tagaliveperiod=20/bestdecisionread=100
long int T;
short int M,N;
long int V,G;
const double eps=1e-8;
const int line=1800;
const double minendurenceerror=0.2;
const int selecttoreadsize=5;
long int maxperiodreadsize=0;
double considernextread;
const long int beginusestrategy=14400;
const long int longmaxobserve=14400;//the higher the more accurate
double controldelay[17]={0};//try to learn-DQ learning
/*controldelay consider the following caused by:
1.must be smaller than its alive period.
2.consider its input frequency and the disk pressure gives.
3.calculate out the frequency of each size and each tag.
*/
const long int sz=100000001;
const double learningrate=0.1;
const int inf=1000;
const long int bestdecisionread=200;//best decision numbers of task for each disks to allocate reading.
long int totalreadtaskscore=0;
double tagaliveperiod[17];//discuss in different tag
bool learnedliveperiod[17];
int readsizecost[10]={0,64,116,158,192,220,243,262,279,295};
//readtimes prediction,how?
long int fre_del[17][4800];
long int fre_write[17][4800];
long int fre_read[17][4800];//use these three data to estimate the next action.
std::unordered_map<int,int>Querytimetask;
//long int allocateread[17];
void PreInput()
{
    //这些数据可以考虑在每一个区间片段内划分一定的空间范围，分别进行读取操作
    //删除价值的评定
    std::cin>>T>>M>>N>>V>>G;
    int x=(T/line)+(T%line>0);
	for(short int i=1;i<=M;i++)//
	{
		for(int j=0;j<x;j++)
		{
			std::cin>>fre_del[i][j];
			fre_del[0][j]+=fre_del[i][j];//decide the probability to be deled.
		}
	}
	for(short int i=1;i<=M;i++)//
	{
		for(int j=0;j<x;j++)
		{
			std::cin>>fre_write[i][j];
			fre_write[0][j]+=fre_write[i][j];
		}
	}
	for(short int i=1;i<=M;i++)//
	{
		for(int j=0;j<x;j++)
		{
			std::cin>>fre_read[i][j];
			fre_read[0][j]+=fre_read[i][j];
		}	
	}
	for(short int i=0;i<=M;i++)
	{
		tagaliveperiod[i]=2*i;
		learnedliveperiod[i]=false;
		//how to manage allocate memory by using the fre_write and fre_read
		/*for(int j=0;j<x;j++)
		{
		tagaliveperiod[i]=std::max(fre_write[i][j]-fre_del[i][j],tagaliveperiod[i]);
		}*/
	}
	for(short int i=1;i<=M;i++)
	{
		maxperiodreadsize=std::max(fre_read[0][i],maxperiodreadsize);
	}
	maxperiodreadsize++;
	//可考虑在此安排策略
	std::cout<<"OK\n";
	//printf("OK\n");
	fflush(stdout);	
}
//what if read and write use G maximum read cost-use dynamic programming in reading,writing,deling process in a time.
class OBS
{
	//使用贪心占用的思想
  private:
  	OBS(){}//contime need to be initialized
  	int f(long int x,bool diff)
  	{
  		//OBS *parent;
  		//std::cout<<"contime "<<contime<<"\n";
  		int timepass=contime-x;
//  		std::cout<<"contime "<<parent->contime<<"\n";
//  		std::cout<<timepass<<"\n";
   	   if(timepass<10)
		 {
		 	return diff?-5:1000-5*(timepass);
		 }
		 else
		 {
		 	if(timepass<105)
		 	{
		 		return diff?-10:1050-10*(timepass);
			 }
			 return 0;
		}	
	}
	struct plocate
	{
		int st,length;
		int startstorepieceid;//note down a series of case pieces--sequence
		bool readpiece;
		bool operator <(const plocate &a)const
		{
			return startstorepieceid<a.startstorepieceid;
		}
	};
	struct locat
	{
		short int disknum;
		int startfrom;
		bool iscontinuous;//是否连续
		std::vector<plocate>saveplace;//如果不连续，给出具体的存储位置
		bool pieceread;
	};
	struct emptyspace
	{
		int start,cons;//starting from and max constance
		bool operator <(const emptyspace &a)const
		{
			if(a.cons==cons)
			{
				return a.start<start;
			}
			return a.cons<cons;//尽可能小的位置优先
		}
	};
  	struct Case
  	{
  		int addtime;//the moment the case write in,be used to calculate the alive period.
  		int id,Size;
		short int tag;
  		locat loc[3];//locate in which disk
  		bool read;//whther there is read task not finished,
  		bool selected_read;//whether there is a task to be read
		bool deled;//whether be recognized as deled
  		std::vector<int>readqueryid;
  		int totalreadvalue;//if one case give many queries,calculate out is important
  		std::queue<emptyspace>Readpieceregion;//Deal with the problem of pieces reading.
  		std::vector<int>unfinishedreadqueryid;//those with no tag to deal with.
  		std::queue<int>delayfinishreadtaskid;//delay finished reading task so as to wait for the next query to gather.
  		//尽可能用连续内存
	};
  	//Case cs[sz];//change into std::map 
  	static std::unordered_map<long int,Case>cs;
	Case csx;
  	struct unread
  	{
  		OBS *parent={};
  		int taskid,caseid,querytime,Size;
  		bool deled;//是否已删
  		bool operator <(const unread &a)const
  		{
  			//return (parent->f(querytime,false)>eps)*(parent->f(querytime,true))*Size<(parent->f(a.querytime,false)>eps)*(parent->f(a.querytime,true))*a.Size;//看看函数如何选择
  			return -querytime*Size<-a.querytime*a.Size;
  		//return getCaseIndex(caseid].totalreadvalue>getCaseIndex(a.caseid].totalreadvalue;
		  //greedy selection
  		//first aid the bigger changer
		}
	};
	struct read
	{
		OBS *parent={};
		int queryid,storeid,start,Size,querytime,locid,caseid;
		int readstart;
		int readturn;
		bool iscontinuous;
		int notcontinuousturn;//if not continuous,label the turn of the pieces.
		short int notcontinuouslocid;
		int deltoaccept;
		//std::vector<plocate>save;
		bool operator <(const read &a)const
		{
			return (start<getDiskIndex(storeid).curdiskpointer)*V+start<(a.start<getDiskIndex(a.storeid).curdiskpointer)*V+a.start;
		}
	};
	unread ud;
	std::priority_queue<unread>U;//change into hand-writing priority_queue to accelerate speed.
	//or change into a more liner 
	struct disk
	{
		int curdiskpointer;//current point disk pointer to which unit
		char lastmove;//note down the last move
		int continuousread; 
		int maxcontinuous;
		short int lastwritetag;
		int maxwritetagcontinuous;
		int readstore;//use to calculate in dynamic programming
		int rest,pretoken;
		std::queue<emptyspace>W;//select randomly-not use priority_queue//最大化连续区间
		//consider separately manage the tasks for each tag.
		std::vector<read>ReadytoRead;
		std::vector<int>ReadPiececheck;
		//std::vector<emptyspace>CaseManager[M];//for M tags use multi manager to read.if read,just move pointer,else just jump.
	};//D[10];
	//std::vector<int>taskidallocate[16];//read when get num.
	static disk D[10];
	std::vector<short int>diskplaceturn;
	bool Addtask[sz];//not repeatively add task to read.
	bool Unusetask[sz];//deal with unuse task.
	static int tagnotenum[17][10];//note the number of each tag in each disks,[0] used to calculate the sum
	static int tagcalcnum[17];
	short int queueopt=0;//选择当前存储的消息队列的编号
	std::queue<int>delaycaseid[2];//构造循环队列
	struct readcase
	{
		//OBS* parent;  //指向外层类实例，防止访问cs报错
		int ch,obj,querytime;//if use read-expanse to control
	};
	readcase rd;
	//readdecision try to use multithread
	//use greedy approach try to low down the runtime.
	void readdecision(int x,int start)//disk id
	{
		
		/*Subsequently Consider reading a copy from different blocks of copy(后面的优化方案：不同块中读取)*/
		OBS *parent={};
		/*modeling in dynamic programming problem,there are N bags with a capacity of G，
		for each backpack ,there is a selectable set S，
	     in each set S，each value of the element has a value of Size*f(time),
	     the cost of getting the element is the sum of point movement and reading
	     each element of a single id could be selected at most once,
	     figure out the maximum profit.
		*/
			//使用多线程，对每一个背包进行动态规划，在状态中标记是否已经read,枚举状态
			//int readsizecost[9]={64,52,42,34,28,23,19,17,16};
		    std::vector<long int>beginarr;
			//in order to select the most efficient order.
			long int beforeselectsize=D[x].ReadytoRead.size();
			auto checkselectcmp=[x](read idx,read jdx)
			{
				/*if(cs[idx.caseid].read&&cs[jdx.caseid].read)
				{*/
					long int costi=(idx.start-D[x].curdiskpointer+(idx.start<D[x].curdiskpointer)*V+readsizecost[std::min(9,idx.Size)]+std::max(idx.Size-9,0)*16);
					long int costj=(jdx.start-D[x].curdiskpointer+(jdx.start<D[x].curdiskpointer)*V+readsizecost[std::min(9,jdx.Size)]+std::max(jdx.Size-9,0)*16);
				double checki=cs[idx.caseid].totalreadvalue*1.0/costi;
				double checkj=cs[jdx.caseid].totalreadvalue*1.0/costj;
				if(abs(checki-checkj)<eps)
				{
					return costi<costj;
				}
				return checki>checkj;
			   // }
			    //return cs[idx.caseid].read&&(!cs[jdx.caseid].read);
			};
			std::vector<read>SelecttoDecide;
			SelecttoDecide=D[x].ReadytoRead;
			sort(SelecttoDecide.begin(),SelecttoDecide.end(),checkselectcmp);
			//std::cout<<"Check overflow:"<<SelecttoDecide[beforeselectsize-1].caseid<<"\n";
			if((beforeselectsize)&&(!cs[SelecttoDecide[beforeselectsize-1].caseid].read))
			{
				while((beforeselectsize)&&(!cs[SelecttoDecide[beforeselectsize-1].caseid].read))
				{
					beforeselectsize--;
				}
			}
			long int targetnum=std::min(bestdecisionread,beforeselectsize-1)*7/10;//70%score
			D[x].ReadytoRead.assign(SelecttoDecide.begin(),SelecttoDecide.begin()+targetnum);
			//std::cout<<D[x].ReadytoRead.size()<<" ";
			struct dpstate
			{
				int profit;
				std::string action;
				int lastread;//the last case read
				int lastcost;
				int readcontinuous;//,lastcost;//continuely read times
				//long int state;//using binary bits to note down each state.//may cost error.
				std::vector<long int>state;
				std::map<long int,bool>Mstate;//check the state that something is in it.
			};
			struct returnstate
			{
				std::string st;
				int re,distance;
			};
			auto checkcost=[x](std::string st,int beforeread)//use in debug
		{
			int readcost[9]={64,52,42,34,28,23,19,17,16};
			long int re=0;
			int curread=beforeread;
			if(getDiskIndex(x).lastmove=='r')
			{
				curread=D[x].continuousread;
			}
			int t=st.length();
			for(int i=0;i<t;i++)
			{
				if(st[i]=='r')
				{
					re+=readcost[std::min(curread,8)];
					curread++;
				}
				else
				{
					re++;
					curread=0;
				}
			}
			return re;
		};
			auto dpreadcost = [parent,x](int readbefore,int i,int j,bool addstring)//调用主函数，在[]内
			{
				//consider G is not enough to read --use j?
				returnstate rt;
				//OBS::readdecision *sk;
				/*calc like y lastreads,x separates,z nextreads.The consdideration below recommend all reads:
			    1.y=3  x=1
                2.y=4,5 x=1,2 			     
			    3.y>=6 x=1,2,3
			     4.z>=2,y+z<=9
				*/
				//dpstate ax;
				int supportsep[7]={0,0,0,1,2,2,3};
				int lastreadsize=0,distanceIJ;
				//distanceIJ:The distance between finishreading I and start reading J
				/*if(i>-1)
				{
				lastreadsize=getDiskIndex(x).ReadytoRead[i].Size;
				}
				else
				{
					lastreadsize=getDiskIndex(x).continuousread;
				}*/
				lastreadsize=readbefore;
				int nextreadsize=getDiskIndex(x).ReadytoRead[j].Size;
				if(i>-1)
				{
				distanceIJ=(getDiskIndex(x).ReadytoRead[j].start+(getDiskIndex(x).ReadytoRead[j].start<getDiskIndex(x).ReadytoRead[i].start)*V)
				-(getDiskIndex(x).ReadytoRead[i].start+getDiskIndex(x).ReadytoRead[i].Size);
			    }
			    else
			    {
			    	distanceIJ=getDiskIndex(x).ReadytoRead[j].start-getDiskIndex(x).curdiskpointer+(getDiskIndex(x).ReadytoRead[j].start<getDiskIndex(x).curdiskpointer)*V;
				}
				rt.re=std::min(readsizecost[std::min(lastreadsize+nextreadsize+distanceIJ,9)]+std::max(0,nextreadsize+lastreadsize+distanceIJ-9)*16-readsizecost[std::min(9,lastreadsize)]-std::max(0,lastreadsize-9)*16,
				readsizecost[std::min(nextreadsize,9)]+std::max(nextreadsize-9,0)*16+distanceIJ);
				bool checkcheap=(rt.re==(readsizecost[std::min(nextreadsize,9)]+std::max(nextreadsize-9,0)*16+distanceIJ));
				//if((lastreadsize<10)&&((nextreadsize>=2)&&(nextreadsize+distanceIJ<10))||(distanceIJ<=supportsep[std::min(6,lastreadsize)]))//return all R
				if(addstring)
				{
				if(!checkcheap)
				{
					//rt.re=readsizecost[std::min(lastreadsize+nextreadsize+distanceIJ,9)]+std::max(0,nextreadsize+lastreadsize+distanceIJ-9)*16-readsizecost[std::min(9,lastreadsize)]-std::max(0,lastreadsize-9)*16;
					for(int i=0;i<nextreadsize+distanceIJ;i++)
					{
						rt.st.push_back('r');
					}
				}
				else //return P
				{
					//rt.re=readsizecost[std::min(nextreadsize,9)]+std::max(nextreadsize-9,0)*16+distanceIJ;
					for(int i=0;i<distanceIJ;i++)
					{
						rt.st.push_back('p');
					}
					for(int i=0;i<nextreadsize;i++)
					{
						rt.st.push_back('r');
					}
				}
				}
				rt.distance=distanceIJ;
				//try all possible?
			    return rt;//in the middle,all of them are passing action
			};
		//std::vector<int>un;
		int total=D[x].ReadytoRead.size();
		//std::sort(D[x].ReadytoRead.begin(),D[x].ReadytoRead.end());
		/*for(int i=0;i<total;i++)
		{

				std::cout<<"Current diskpointer: "<<D[x].curdiskpointer<<"\n";
			std::cout<<"Check read:"<<D[x].ReadytoRead[i].queryid<<" "<<D[x].ReadytoRead[i].caseid<<" "<<cs[D[x].ReadytoRead[i].caseid].totalreadvalue<<" "<<D[x].ReadytoRead[i].start<<"\n";
		}*///use in debug
		//dpstate dp[2*G+1][total];
		//std::vector<std::vector<dpstate>>dp(2*G+1,std::vector<dpstate>(total));
		dpstate dp[G+1];
        //memcpy(dp,sizeof(dp),0);
		dp[0].lastread=-1;
		dp[0].profit=0;
		dp[0].action="";
		dp[0].lastcost=0;
		dp[0].readcontinuous=D[x].continuousread;
		dp[0].state=beginarr;
		dp[G].action="";
		/*if(D[x].lastmove!='r')
		{
		dp[0].lastread=-1;
		}
		else
		{
			dp[0].lastread=(D[x].curdiskpointer<D[x].continuousread)*V+D[x].curdiskpointer-D[x].continuousread;
		}*/
		//could consider slope optimization in dp- cost函数是凸函数
		//dp consideration or search optimize
		/*
	    for(int i=1;i<=G;i++)
		{
			//if j start from dp[i-1].lastread+1?
			for(int j=0;j<total;j++)
			{
			dp[i][j]=dp[i-1][j];
			int chosen=-1;
		       for(int k=0;k<j;k++)
				{
				int cost=dpreadcost(k,j).re;
				if(i>=cost)
				{
					dp[i][j].profit=std::max(dp[i][j].profit,dp[i-cost][j].profit+getCaseIndex(getDiskIndex(x).ReadytoRead[k].caseid).totalreadvalue);
					if(dp[i][j].profit==dp[i-cost][j].profit+getCaseIndex(getDiskIndex(x).ReadytoRead[k].caseid).totalreadvalue)
					{
						chosen=k;
					}
				}
				}
				if(chosen>-1)
			{
				//std::cout<<i<<" "<<dpreadcost(dp[i-1].lastread,chosen).re<<"\n";
				dp[i][j].action=dp[i-dpreadcost(j,chosen).re][j].action+(dpreadcost(j,chosen).st);
				//dp[i][j].state=dp[i-dpreadcost(j,chosen).re][j].state+(1<<chosen);
				dp[i][j].state=dp[i-dpreadcost(j,chosen).re].state;
				dp[i][j].state.push_back(chosen);
				//dp[i][j].state=dp[i-dpreadcost(j,chosen).re].state;
				//dp[i][j].state.push_back(chosen);
				dp[i][j].lastread=chosen;
			}
			}
		}
		*/
		//管理0-1背包，如何将物品存入，记录状态，每个物品只能取一次
		//考虑集合dp
		/*for(int i=1;i<=G;i++)
		{
			dp[i]=dp[i-1];
			int chosen=-1;
			//if j start from dp[i-1].lastread+1?
			for(int j=0;j<total;j++)
			{
				if(!cs[D[x].ReadytoRead[j].caseid].read)
				{
					continue;
				}
				//if(cs[D[x].ReadytoRead[j].caseid].)//if read in piece
				returnstate rt=dpreadcost(dp[i-1].readcontinuous,dp[i-1].lastread,j);
				int cost=rt.re;
				if(i-dp[i-1].lastcost>=cost)
				{
					if(dp[i].profit<dp[i-cost].profit+getCaseIndex(getDiskIndex(x).ReadytoRead[j].caseid).totalreadvalue)
					{
						chosen=j;
					}
					dp[i].profit=std::max(dp[i].profit,dp[i-cost].profit+getCaseIndex(getDiskIndex(x).ReadytoRead[j].caseid).totalreadvalue);
				}
			}
			if((chosen>-1)&&(chosen>dp[i-1].lastread))//&&(!dp[i-dpreadcost(dp[i-1].readcontinuous,dp[i-1].lastread,chosen).re].Mstate[chosen]))//问题在于：记录的时候出现问题
			{
				//std::cout<<i<<" "<<dpreadcost(dp[i-1].lastread,chosen).re<<"\n";
				//std::cout<<i<<" "<<D[x].ReadytoRead[chosen].queryid<<"\n";
				returnstate rt=dpreadcost(dp[i-1].readcontinuous,dp[i-1].lastread,chosen);
				/*std::cout<<"Current cost: "<<i<<"\n";
				if(dp[i-1].lastread==-1)
				{
					std::cout<<D[x].ReadytoRead[chosen].caseid<<" Distance to diskpointer:"<<rt.distance<<",with a cost of "<<rt.re<<"\n";
				}
				else
				{
				std::cout<<"Distance between "<<D[x].ReadytoRead[dp[i-1].lastread].caseid<<" and "<<D[x].ReadytoRead[chosen].caseid<<" : "<<rt.distance
				<<",with a cost of "<<rt.re<<"\n";
			    }*/
				/*char headcheck=rt.st[0];
				dp[i].action=dp[i-dp[i-1].lastcost-rt.re].action;
				dp[i].action=dp[i].action+rt.st;
				//dp[i].state=dp[i-dpreadcost(dp[i-1].lastread,chosen).re].state+(1<<chosen);
				//dp[i].state.clear();
				//dp[i].state=dp[i-dp[i-1].lastcost-rt.re].state;
				//dp[i].state.push_back(chosen);
				dp[i].lastcost=dp[i-1].lastcost+rt.re;
				//dp[i].Mstate=dp[i-dp[i-1].lastcost-rt.re].Mstate;
				//dp[i].Mstate[chosen]=true;
				if(headcheck=='r')
				{
					dp[i].readcontinuous+=rt.distance+D[x].ReadytoRead[chosen].Size;
				}
				else
				{
					dp[i].readcontinuous=D[x].ReadytoRead[chosen].Size;
				}
				//dp[i].state=dp[i--dpreadcost(dp[i-1].lastread,chosen).re].state;
				//dp[i].state.push_back(chosen);
				dp[i].lastread=chosen;
			}
		}*/
		//take turns to select method
		long int totalcost=0,lastread=-1,lastreadcontinuous=D[x].continuousread;
		sort(D[x].ReadytoRead.begin(),D[x].ReadytoRead.end());
		for(long int i=0;i<targetnum;i++)
		{
			if(!getCaseIndex(D[x].ReadytoRead[i].caseid).read)
			{
			continue;
			}
			returnstate rt=dpreadcost(lastreadcontinuous,lastread,i,false);
			if(totalcost==G)
			{
				break;
			}
			if((totalcost+rt.re)>G)
			{
				continue;
				//break;
			}
			rt=dpreadcost(lastreadcontinuous,lastread,i,true);
			char checkhead=rt.st[0];
			if(checkhead=='r')
			{
				lastreadcontinuous+=D[x].ReadytoRead[i].Size+rt.distance;
			}
			else
			{
				lastreadcontinuous=D[x].ReadytoRead[i].Size;
			}
			totalcost+=rt.re;
			dp[G].action+=rt.st;
			lastread=i;
			getCaseIndex(D[x].ReadytoRead[i].caseid).read=false;
		    
		}
		//greedy select method.
		/*int selectid=0,totalcost=0,lastread=-1,currentdiskpointer=D[x].curdiskpointer,lastreadcontinuous=D[x].continuousread;
		auto greedyreadselection=[](const read &rda,const read &rdb)
		{
		if(abs(getCaseIndex(rda.caseid).totalreadvalue*exp(-rda.deltoaccept)-getCaseIndex(rdb.caseid).totalreadvalue*exp(-rdb.deltoaccept))<eps)
			{
				return rda.readstart+readsizecost[std::min(9,rda.Size)]+std::max(0,rda.Size-9)*16<rdb.readstart+readsizecost[std::min(9,rdb.Size)]+std::max(0,rdb.Size-9)*16;
			}
			return getCaseIndex(rda.caseid).totalreadvalue*exp(-rda.deltoaccept)>getCaseIndex(rdb.caseid).totalreadvalue*exp(-rdb.deltoaccept);
		};
		//std::priority_queue<read,std::vector<read>,decltype(greedyreadselection)>Decisiontoread(greedyreadselection);
		std::vector<read>Choosetoreadselection;
		int mincost=D[x].ReadytoRead[0].readstart-D[x].curdiskpointer+readsizecost[std::min(9,D[x].ReadytoRead[0].Size)]+std::max(0,D[x].ReadytoRead[0].Size-9)*16;
		for(long int i=0;i<total;i++)
		{
			D[x].ReadytoRead[i].readturn=i;
			mincost=std::min(mincost,D[x].ReadytoRead[0].readstart-D[x].curdiskpointer+readsizecost[std::min(9,D[x].ReadytoRead[0].Size)]+std::max(0,D[x].ReadytoRead[0].Size-9)*16);
		}
		auto checkisplacable=[x,dpreadcost](std::vector<read> *Choosetoreadselection,int *totalcost,int asknum)//if one cost need to add to the queue-sort in arrangement.
		{
		  int l=0,r=(*Choosetoreadselection).size()-1,bigsize=r+1;
		  int selectplace=0;
		  long int mid;
	      if(!(*Choosetoreadselection).empty())
		  {
		  while(l<r)
		  {
		  	mid=(l+r)/2;
		  	if((*Choosetoreadselection)[mid].readstart>D[x].ReadytoRead[asknum].readstart)
		  	{
		  	r=mid-1;	
			}
			else
			{
				l=mid;
			}
		  };
		  selectplace=l;
		  }
		  //mid=l;
		  //selectplace=mid+((*Choosetoreadselection)[mid].readstart<D[x].ReadytoRead[asknum].readstart);}
		  returnstate rt,rq,origin;
		  if((selectplace!=bigsize+1)&&(selectplace))
		  	{
		  	rt=dpreadcost(D[x].continuousread,(*Choosetoreadselection)[mid].readturn,D[x].ReadytoRead[asknum].readturn,false);
		  	rq=dpreadcost(D[x].continuousread,D[x].ReadytoRead[asknum].readturn,(*Choosetoreadselection)[mid+1].readturn,false);
		  	origin=dpreadcost(D[x].continuousread,(*Choosetoreadselection)[mid].readturn,(*Choosetoreadselection)[mid+1].readturn,false);
		    }
		    else
		    {
		    	if(!selectplace)
		    	{
		    		rt=dpreadcost(D[x].continuousread,-1,D[x].ReadytoRead[asknum].readturn,false);
				}
				else
		    	{
                    rt=dpreadcost(D[x].continuousread,(*Choosetoreadselection)[bigsize].readturn,D[x].ReadytoRead[asknum].readturn,false);
				}
				rq.re=0;
		    	origin.re=0;
			}
		  if((*totalcost)+rt.re+rq.re-origin.re<=G)
		  {
		  	(*totalcost)+=rt.re+rq.re-origin.re;
			if((!(*Choosetoreadselection).empty())&&(selectplace!=(*Choosetoreadselection).size()))
			{
				(*Choosetoreadselection).push_back(D[x].ReadytoRead[asknum]);
			}
			else
		  	{
			  (*Choosetoreadselection).insert((*Choosetoreadselection).begin()+selectplace,D[x].ReadytoRead[asknum]);
		    }
		  	//dp[G].action.insert(rt.st+rf.st,dp[G].action.begin()+D[x].curdiskpointer);
		  	return true;
		  }
		  return false;
		};
		/*for(long int i=0;i<std::min(selecttoreadsize,total);i++)
		{
			//Decisiontoread.push(D[x].ReadytoRead[i]);
			selectid++;
		}*/
		/*bool addtopq=false;
		int visitbutnotchoose=0;
		read rg;
			while((totalcost<=G)&&(selectid<total))//how to end?
			{
				//rg=Decisiontoread.top();
				//Decisiontoread.pop();
				rg=D[x].ReadytoRead[selectid];
				selectid++;
				//sort in arrange->to minimize the cost so as to ;
				//returnstate rt=dpreadcost(lastread,rg.);
				if(!cs[rg.caseid].read)
				{
					continue;
				}
				if(!checkisplacable(&Choosetoreadselection,&totalcost,rg.readturn))
				{
					rg.deltoaccept++;
					//SelectoDecide.push(rd);
					visitbutnotchoose++;
				}
				/*if(visitbutnotchoose>=SelecttoDecide.size())//add to priority queue
				{
					addtopq=true;
					if(selectid==total)
					{
						break;
					}
					for(long int i=selectid;i<std::min(selectid+selecttoreadsize,total);i++)
					{
						SelecttoDecide.push_back(D[x].ReadytoRead[i]);
					}
					selectid+=selecttoreadsize;
					addtopq=false;
				}*/
			/*}
			long int choosereadsize=Choosetoreadselection.size();
			for(long int i=0;i<choosereadsize;i++)
			{
				returnstate rt=dpreadcost(lastreadcontinuous,lastread,Choosetoreadselection[i].readturn,true);
				dp[G].action+=rt.st;
				char checkhead=rt.st[0];
			if(checkhead=='r')
			{
				lastreadcontinuous+=D[x].ReadytoRead[i].Size+rt.distance;
			}
			else
			{
				lastreadcontinuous=D[x].ReadytoRead[i].Size;
			}
				lastread=Choosetoreadselection[i].readturn;
				cs[Choosetoreadselection[i].caseid].read=false;
			}*/
		D[x].continuousread=lastreadcontinuous;
		//dp[2*G]-answer
		//activation thought by Deepseek LLM-state compression dp
		int k=0;
		long int s=dp[G].action.length();
		/*if(checkcost(dp[G].action,D[x].continuousread)>G)//debug
		{
			std::cout<<checkcost(dp[G].action,D[x].continuousread)<<" "<<G<<"\n";
			std::cout<<"Disk "<<x<<" overflows its total cost."<<"\n";
			exit(-1);
		}*/
		//std::cout<<"Check:"<<Choosetoreadselection.size()<<std::endl;
		//Choosetoreadselection.clear();
		if(s||(!D[x].ReadytoRead.size()))
		{
			//std::cout<<s<<std::endl;
		//std::cout<<x<<" "<<getDiskIndex(x).curdiskpointer<<"\n";
		//printf("%s",dp[G].action);
		std::cout<<dp[G].action<<"#\n";
		//printf("#\n");
		/*for(long int i=0;i<=s;i++)
		{
			if(i==s)
			{
				//std::cout<<"#";
				//printf("#");
			}
			else
			{
			//std::cout<<dp[G].action[i];
			}
		}*/
		//std::cout<<"\n";//use in debug
		/*long int g=G;
		/*while(g>0)
		{
			if(dp[g].lastread==dp[g-1].lastread)
			{
				g--;
			}
			else
			{
				getCaseIndex(D[x].ReadytoRead[dp[g].lastread].caseid).read=false;
				g-=dp[g].lastcost;
			}
		}*/
		D[x].lastmove=dp[G].action[s-1];
		long int l=s-1;
		/*if(s&&dp[G].action[s-1]=='r')
		{
			/*while(dp[G].action[l]=='r')
			{
				l--;
			}*/
			//D[x].continuousread=s-l;
			/*D[x].continuousread=lastreadcontinuous;
		}
		else
		{
			if(s){D[x].continuousread=0;}
		}//else not change
		//long int y=dp[G].state.size();
		/*for(long int i=0;i<x;i++)
		{
			dp[G].state[i]
		}*/
		/*while(k<total)
		{
			//if not read,push into unr
			if((dp[G].state>>k)%2==1)
			{
				getCaseIndex(D[x].ReadytoRead[k].caseid).read=false;
			}
			k++;
		}*/
		//long int hasread=dp[G].state.size();
		/*for(long int i=0;i<hasread;i++)
		{
			if(D[x].ReadytoRead[dp[G].state[i]].iscontinuous)
			{
				bool checkreallyread=true;
				/*for(long int i=0;i<cs[D[x].ReadytoRead[dp[G].state[i]].caseid].Size;i++)
				{
					if(dp[G].state[((D[x].ReadytoRead[dp[G].state[i]].start+i)<getDiskIndex(x).curdiskpointer)*V
					+D[x].ReadytoRead[dp[G].state[i]].start+i-getDiskIndex(x).curdiskpointer]!='r')
					{
						checkreallyread=false;
						break;
					}
				}*/
				/*if(checkreallyread)
				{
				getCaseIndex(D[x].ReadytoRead[dp[G].state[i]].caseid).read=false;
			    }
			}
			/*else
			{
	            getCaseIndex(D[x].ReadytoRead[dp[G].state[i]].caseid).loc[notcontinuouslocid].saveplace[notcontinuousturn].readpiece=true;
	            getCaseIndex(D[x].ReadytoRead[dp[G].state[i]].caseid)
			}*/
		//}
		getDiskIndex(x).curdiskpointer=(getDiskIndex(x).curdiskpointer+s)%V;//move
		//deal with those who are in pieces.
		}
		else//use jump method
		{
			int jumpchosen=0;
			int maxreadvaluechosen=0;
			for(int i=0;i<total;i++)
			{
				if(cs[D[x].ReadytoRead[i].caseid].totalreadvalue>maxreadvaluechosen)
				{
					jumpchosen=i;
				}
				maxreadvaluechosen=std::max(maxreadvaluechosen,cs[D[x].ReadytoRead[i].caseid].totalreadvalue);
			}
			std::cout<<"j "<<D[x].ReadytoRead[jumpchosen].start+1;
			//printf("j %d\n",D[x].ReadytoRead[jumpchosen].start+1);
			std::cout<<"\n";//use in debug
			D[x].curdiskpointer=D[x].ReadytoRead[jumpchosen].start;
			D[x].lastmove='j';
			D[x].continuousread=0;			
		}
		D[x].ReadytoRead.clear();
		//return un;
    }
	void write(long int x)
	{
		int cp=0,tx=0;//copy 3 pieces
		std::queue<emptyspace>E;//缓存
		emptyspace es,eq;
		//shouldn't be equal to the same disks
		bool savedisk[10];
		for(int i=0;i<10;i++)
		{
			savedisk[i]=false;
		}
		auto cmpdisk=[x](short int i,short int j)
		{
			/*if(tagnotenum[getCaseIndex(x).tag][0]&&(tagnotenum[getCaseIndex(x).tag][i]&&tagnotenum[getCaseIndex(x).tag][j]))//if there is tag saved,use this to decide
			{
			   //catch the place to search get continuous task
			   //to avoid the resources be attached to each other,separate maybe a good choice
			   //then compare just like below.
			   if(tagnotenum[getCaseIndex(x).tag][i]==tagnotenum[getCaseIndex(x).tag][j]) 
			   {
			   return i<j;
			   }
			   return tagnotenum[getCaseIndex(x).tag][i]<tagnotenum[getCaseIndex(x).tag][j];//i saved and j not saved
			}
			if(tagnotenum[getCaseIndex(x).tag][i]||tagnotenum[getCaseIndex(x).tag][j])
			{
				return (!tagnotenum[getCaseIndex(x).tag][i])&&(tagnotenum[getCaseIndex(x).tag][j]);
			}
			if(D[i].rest==D[j].rest)
			{
				if(D[i].maxcontinuous==D[j].maxcontinuous)
				{return i<j;}
				return D[i].maxcontinuous>D[j].maxcontinuous;
			}*/
			if(!((D[i].lastwritetag==(getCaseIndex(x).tag))^(D[j].lastwritetag==(getCaseIndex(x).tag))))
			{
				if((D[i].lastwritetag==getCaseIndex(x).tag)&&(D[i].maxwritetagcontinuous==D[j].maxwritetagcontinuous))
				{
	                return  D[i].maxwritetagcontinuous>D[j].maxwritetagcontinuous;
				}
			return D[i].rest>D[j].rest;
			}
			return (D[i].lastwritetag==(getCaseIndex(x).tag))&&(!(D[i].lastwritetag==getCaseIndex(x).tag));
		};
		std::sort(diskplaceturn.begin(),diskplaceturn.end(),cmpdisk);
		//try possible not to save in only 3 disks
		std::vector<plocate>Pt;
		std::vector<plocate>resultpt;
		plocate pl;
		auto cmpplocate=[](plocate it,plocate jt)
		{
			return it.st<jt.st;
		};
		while(cp<3)
		{
			for(short int i=0;i<N;i++)
			{
				if((!savedisk[diskplaceturn[i]])&&(D[diskplaceturn[i]].rest>=cs[x].Size))
				{
					tagnotenum[getCaseIndex(x).tag][diskplaceturn[i]]++;
				    tagnotenum[getCaseIndex(x).tag][0]++;
				    cs[x].loc[cp].disknum=diskplaceturn[i];
					D[diskplaceturn[i]].rest-=cs[x].Size;
					savedisk[diskplaceturn[i]]=true;
					int rst=cs[x].Size;
					while(rst)
					{
						es=D[diskplaceturn[i]].W.front();
						D[diskplaceturn[i]].W.pop();
						int re=std::min(rst,es.cons);
						rst-=re;
						pl.st=es.start;
						pl.length=re;
						Pt.push_back(pl);
						es.cons-=re;
						es.start+=re;
						if(es.cons)
						{
							D[diskplaceturn[i]].W.push(es);
						}
					}
					if(Pt.size()>1)
					{
						long int ptsize=Pt.size();
						std::sort(Pt.begin(),Pt.end(),cmpplocate);
						long int rpt=0;
						for(long int i=0;i<ptsize;i++)
						{
							if(i==0)
							{
								resultpt.push_back(Pt[i]);
							}
							else
							{
								if(Pt[i].st==resultpt[rpt].st+resultpt[rpt].length)
								{
									resultpt[rpt].length+=Pt[i].length;
								}
								else
								{
									resultpt.push_back(Pt[i]);
									rpt++;
								}
							}
						}
					}
					if((resultpt.size()==1)||(Pt.size()==1))
					{
						if(resultpt.empty())
						{
						resultpt.push_back(Pt[0]);
						}
						cs[x].loc[cp].iscontinuous=true;
						cs[x].loc[cp].startfrom=resultpt[0].st;
					}
					else
					{
						cs[x].loc[cp].iscontinuous=false;
						long int resultptsize=resultpt.size();
						for(long int j=0;j<resultptsize;j++)
						{
							cs[x].loc[cp].saveplace.push_back(resultpt[j]);
						}
					}
					resultpt.clear();
					Pt.clear();
					es=D[diskplaceturn[i]].W.front();
					D[diskplaceturn[i]].maxcontinuous=es.cons;
					/*if(!cs[x].loc[cp].iscontinuous)
					{
						D[diskplaceturn[i]].maxwritetagcontinuous=0;
					}*/
					if(cs[x].loc[cp].iscontinuous)
					{
						if(D[diskplaceturn[i]].lastwritetag==getCaseIndex(x).tag)
						{
						D[diskplaceturn[i]].maxwritetagcontinuous+=cs[x].Size;
						}
						else
						{
							D[diskplaceturn[i]].maxwritetagcontinuous=cs[x].Size;
						}
						D[diskplaceturn[i]].lastwritetag=getCaseIndex(x).tag;
					}
					//D[diskplaceturn[i]].lastwritetag=getCaseIndex(x).tag;
					break;
				}
			}
			cp++;
		}
		//previous:thought
		//first search: try to manage continuous.search for the smallest piece to store;if not,then select the disks that could save whether in pieces or not.
		//use bool-checkplace to recognize whether is saved in the previous approach.
	}
	void del(std::vector<long int> *arr,long int x)
	{
		OBS *parent={};
		//previous-thought:try to recover its entity compound to the pieces-find out the pieces,find out in the whole priority_queue.
		if(getCaseIndex(x).read||getCaseIndex(x).unfinishedreadqueryid.size()||getCaseIndex(x).delayfinishreadtaskid.size())//未完成任务
		{
			arr->push_back(x);
		}
		emptyspace es;
		for(int i=0;i<3;i++)
		{
			tagnotenum[getCaseIndex(x).tag][getCaseIndex(x).loc[i].disknum]--;
				tagnotenum[getCaseIndex(x).tag][0]--;
			if(getCaseIndex(x).loc[i].iscontinuous)
			{
				es.start=getCaseIndex(x).loc[i].startfrom;
				es.cons=getCaseIndex(x).Size;
				D[getCaseIndex(x).loc[i].disknum].W.push(es);
				//delfunc(i,-1,getCaseIndex(x).loc[i].iscontinuous);
			}
			else
			{
				long int g=getCaseIndex(x).loc[i].saveplace.size();
				for(long int j=0;j<g;j++)
				{
					es.start=getCaseIndex(x).loc[i].saveplace[j].st;
					es.cons=getCaseIndex(x).loc[i].saveplace[j].length;
					D[getCaseIndex(x).loc[i].disknum].W.push(es);
					//delfunc(i,j,getCaseIndex(x).loc[i].iscontinuous);
				}
			}
			D[getCaseIndex(x).loc[i].disknum].rest+=getCaseIndex(x).Size;
			emptyspace es=D[getCaseIndex(x).loc[i].disknum].W.front();
			D[getCaseIndex(x).loc[i].disknum].maxcontinuous=es.cons;
		}			
	}
		
	std::queue<readcase>R;
  public:
  	static int contime;
  	//static std::map<long int,Case> &getmap(){return cs;}
  	static disk &getDiskIndex(int index){return D[index];}
	static Case &getCaseIndex(int index){return cs[index];}
  	void Initial()//全局初始
  	{
  		emptyspace es;
  		for(short int i=0;i<N;i++)
  		{
  		D[i].rest=V;	
  		D[i].curdiskpointer=0;
  		D[i].lastmove='j';
  		D[i].continuousread=0;
  		D[i].readstore=0;
  		D[i].maxcontinuous=V;
  		D[i].lastwritetag=0;
  		D[i].maxwritetagcontinuous=0;
  		es.start=0;
  		es.cons=V;
  		D[i].W.push(es);
  		diskplaceturn.push_back(i);
  		for(short int j=0;j<16;j++)
		{
			tagnotenum[j][i]=0;
		}
		}
		for(long int i=0;i<=sz;i++)
		{
			Addtask[i]=false;
			Unusetask[i]=false;
		}
	}
  void delInput(int delnum)//delete operation input
  {
  	OBS *parent={};
  	std::vector<long int>Unr;
  	std::vector<long int>Unrabort[2];
	for(int i=0;i<delnum;i++)
	{
		int delopt;
		std::cin>>delopt;
		if(contime<=longmaxobserve)//update with learning
		{
			long int livetime=contime-getCaseIndex(delopt).addtime;
			double err=abs(livetime-tagaliveperiod[getCaseIndex(delopt).tag])/tagaliveperiod[getCaseIndex(delopt).tag];
			if(err<minendurenceerror)
			{
				tagaliveperiod[getCaseIndex(delopt).tag]=(livetime+tagaliveperiod[getCaseIndex(delopt).tag])*1.0/2;
			}
			else
			{
			if(learnedliveperiod[getCaseIndex(delopt).tag])//make a deal with 
			{
				tagaliveperiod[getCaseIndex(delopt).tag]-=learningrate*(livetime-tagaliveperiod[getCaseIndex(delopt).tag]);//gradient method to deal with 
			}
			else//refuse the previous assumption
			{
				tagaliveperiod[getCaseIndex(delopt).tag]=livetime;
			}
			}
			learnedliveperiod[getCaseIndex(delopt).tag]=true;
			controldelay[getCaseIndex(delopt).tag]=std::min(std::min(tagaliveperiod[getCaseIndex(delopt).tag],35.0),tagaliveperiod[getCaseIndex(delopt).tag]/5);
		}
		fre_read[getCaseIndex(delopt).tag][contime/1800+(contime%1800>0)]-=getCaseIndex(delopt).Size;
		//scanf("%d",&delopt);
		getCaseIndex(delopt).deled=true;
		tagcalcnum[getCaseIndex(delopt).tag]-=csx.Size;
		del(&Unr,delopt);
	}
	long int unrsize=Unr.size();
	long int n_abort=0;
	for(long int i=0;i<unrsize;i++)
	{
		long int c=cs[Unr[i]].readqueryid.size();//return unread query for each unfinished read case
		long int d=cs[Unr[i]].unfinishedreadqueryid.size();
		long int e=cs[Unr[i]].delayfinishreadtaskid.size();
		Unrabort[0].push_back(c);
		Unrabort[1].push_back(d);
		std::set<int>S;
		for(long int j=0;j<c;j++)
		{
			S.insert(cs[Unr[i]].readqueryid[j]);
		}
		for(long int j=0;j<d;j++)
		{
			S.insert(cs[Unr[i]].unfinishedreadqueryid[j]);
		}
		for(long int j=0;j<e;j++)
		{
			int query=cs[Unr[i]].delayfinishreadtaskid.front();
			cs[Unr[i]].delayfinishreadtaskid.pop();
			S.insert(query);
			cs[Unr[i]].delayfinishreadtaskid.push(query);
		}
	   //n_abort+=c+(c==0)*d;
	   n_abort+=S.size();
	   S.clear();	
	}
	std::cout<<n_abort<<"\n";
	for(long int i=0;i<unrsize;i++)
	{
		for(long int j=0;j<Unrabort[0][i];j++)
		{
			if(!Unusetask[cs[Unr[i]].readqueryid[j]])
		{
		std::cout<<cs[Unr[i]].readqueryid[j]<<"\n";
		}
		}
		for(long int j=0;j<Unrabort[1][i];j++)
		{
		std::cout<<cs[Unr[i]].unfinishedreadqueryid[j]<<"\n";
		}
		while(!cs[Unr[i]].delayfinishreadtaskid.empty())
		{
			int query=cs[Unr[i]].delayfinishreadtaskid.front();
			cs[Unr[i]].delayfinishreadtaskid.pop();
			std::cout<<query<<"\n";
		}
		cs[Unr[i]].readqueryid.clear();
		cs[Unr[i]].unfinishedreadqueryid.clear();
	}
	fflush(stdout);
	Unr.clear();
	Unrabort[0].clear();
	Unrabort[1].clear();
  } 
  void writeInput(int writenum)//write operation input
  {
  	std::vector<int>Ws;
  	auto cmp=[](int a,int b)
  	{
  		//OBS *parent={};
  		if(tagcalcnum[getCaseIndex(a).tag]==tagcalcnum[getCaseIndex(b).tag])
  	{
	  if(getCaseIndex(a).Size==getCaseIndex(b).Size)
	  {
	  	return getCaseIndex(a).id<getCaseIndex(b).id;
	  }	
	  return getCaseIndex(a).Size>getCaseIndex(b).Size;	
	  }
	  return tagcalcnum[getCaseIndex(a).tag]>tagcalcnum[getCaseIndex(b).tag];
	};
	for(int i=0;i<writenum;i++)
	{ 
		std::cin>>csx.id>>csx.Size>>csx.tag;
		csx.addtime=contime;
		fre_write[csx.tag][contime/1800+(contime%1800>0)]-=csx.Size;
		tagcalcnum[csx.tag]+=csx.Size;
		csx.deled=false;
		csx.read=false;
		Ws.push_back(csx.id);
		cs[csx.id]=csx;
		//std::cout<<cs[csx.id].id<<" "<<cs[csx.id].Size<<"\n";
		//allocate memory
	}
	sort(Ws.begin(),Ws.end(),cmp);
	//greedy write in an order of Size
	for(int i=0;i<writenum;i++)
	{
		write(Ws[i]);
		std::cout<<Ws[i]<<"\n";
		for(int j=0;j<3;j++)
		{
			std::cout<<cs[Ws[i]].loc[j].disknum+1<<" ";
			if(cs[Ws[i]].loc[j].iscontinuous)//continuous output
			{
				std::cout<<cs[Ws[i]].loc[j].startfrom+1<<" ";
				for(int k=1;k<cs[Ws[i]].Size;k++)
				{
					std::cout<<cs[Ws[i]].loc[j].startfrom+k+1<<" ";
				}
			}
			else
			{
				long int x=cs[Ws[i]].loc[j].saveplace.size();
				for(long int k=0;k<x;k++)
				{
					for(long int l=0;l<cs[Ws[i]].loc[j].saveplace[k].length;l++)
					{
						std::cout<<cs[Ws[i]].loc[j].saveplace[k].st+l+1<<" ";
					}
				}
			}
			std::cout<<"\n";
		}
	}
	fflush(stdout);
  }
  void readInput(int readnum)//read operation input
  {
  	//同一个时间片内多个读取操作怎么办？用bool 
  	//consider those cases save in pieces -if pieces larger than 2,put in as a case,else read it by accidentally
  	OBS *parent={};
  	read rf;
  	unread unr,unq;
  	//unordered_map<int,int>M;
  	std::vector<readcase>Qread;
  	std::priority_queue<unread>UQuery[M+1];
  	int querytag[M+1]={0};
  	int qreadnum=0;
  	std::vector<int>finishread;
  	long int n_rsp=0;
	for(int i=0;i<readnum;i++)
	{
		int ch,obj;
		std::cin>>rd.ch>>rd.obj;
		Querytimetask[rd.ch]=contime;
		fre_read[getCaseIndex(rd.obj).tag][contime/1800+(contime%1800>0)]-=getCaseIndex(rd.obj).Size;
		querytag[getCaseIndex(rd.obj).tag]++;
		if((!getCaseIndex(rd.obj).delayfinishreadtaskid.empty())&&(contime>beginusestrategy))
		{
			getCaseIndex(rd.obj).delayfinishreadtaskid.push(rd.ch);
			continue;
		}
		//scanf("%d%d",&rd.ch,&rd.obj);
		cs[rd.obj].read=true;
		//totalreadtaskscore+=cs[rd.obj].Size;
		unr.taskid=rd.ch;
		unr.caseid=rd.obj;
		unr.querytime=contime;
		unr.Size=getCaseIndex(rd.obj).Size;
		//std::cout<<unr.Size<<"\n";
		unr.deled=false;
		//use a new queue to select.
		//UQuery[getCaseIndex(rd.obj).tag].push(unr);
		//push into Unread to select
		U.push(unr);
		//previous-thought:continuous if immediately push into Query,or the size control in the manage.  
		//read data
	}
	while(!delaycaseid[queueopt].empty())
	{
		int querycase=delaycaseid[queueopt].front();
		delaycaseid[queueopt].pop();
		int querycasetaskid=getCaseIndex(querycase).delayfinishreadtaskid.front();
		if((Querytimetask[querycasetaskid]+(controldelay[getCaseIndex(querycase).tag]/getCaseIndex(querycase).Size)>=contime)
		||(tagaliveperiod[getCaseIndex(querycase).tag]-(controldelay[getCaseIndex(querycase).tag]/getCaseIndex(querycase).Size)+getCaseIndex(querycase).addtime<=contime))//value enough to release,or has a risk of being deled
		{
			while(!getCaseIndex(querycase).delayfinishreadtaskid.empty())
			{
				int queryid=getCaseIndex(querycase).delayfinishreadtaskid.front();
				getCaseIndex(querycase).delayfinishreadtaskid.pop();
				finishread.push_back(queryid);
				n_rsp++;
			}
		}
		else
		{
			delaycaseid[!queueopt].push(querycase);
		}
	}
	queueopt=(!queueopt);
	//pushing unread case into the ReadytoRead vector
	std::queue<unread>TempU;
	//std::cout<<contime<<" Unread case num:"<<U.size()<<"\n";
	while(!U.empty())//allocate reading task
	{
		unr=U.top();
		U.pop();
		if(!getCaseIndex(unr.caseid).deled)//&&f(unr.querytime,false))
		{
			if(!Addtask[unr.taskid])
			{
			getCaseIndex(unr.caseid).readqueryid.push_back(unr.taskid);
		    Addtask[unr.taskid]=true;
			}
		rd.ch=unr.taskid;
		rd.obj=unr.caseid;
		rd.querytime=unr.querytime;
		Qread.push_back(rd);
		qreadnum++;
		//if under controll of bestreading decision
		if(contime-unr.querytime<105)
		{
		//if((f(contime-unr.querytime,false)*1.0/1000*unr.Size>0.800)||(getCaseIndex(unr.caseid).totalreadvalue))
		//{
		bool beplaced=false;
			if(!getCaseIndex(unr.caseid).selected_read)
		{
	     short int baseselect=-1;
	     long int bs;
		for(int j=0;j<3;j++)
		{
			if((cs[unr.caseid].loc[j].iscontinuous)
			//&&(D[getCaseIndex(unr.caseid).loc[j].disknum].ReadytoRead.size()<bestdecisionread*3/2+(bestdecisionread)%2)
			/*&&(getCaseIndex(unr.caseid).loc[j].startfrom-D[getCaseIndex(unr.caseid).loc[j].disknum].curdiskpointer
			+(getCaseIndex(unr.caseid).loc[j].startfrom<D[getCaseIndex(unr.caseid).loc[j].disknum].curdiskpointer)*V
			+readsizecost[std::min(9,unr.Size)]+std::max(unr.Size-9,0)*16<=G)*///to previate from impossible element.
			)
			{
			  if((baseselect==-1)||
			  ((getCaseIndex(unr.caseid).loc[j].startfrom<D[getCaseIndex(unr.caseid).loc[j].disknum].curdiskpointer)*V+(getCaseIndex(unr.caseid).loc[j].startfrom-D[getCaseIndex(unr.caseid).loc[j].disknum].curdiskpointer)<
			  (getCaseIndex(unr.caseid).loc[baseselect].startfrom<D[getCaseIndex(unr.caseid).loc[baseselect].disknum].curdiskpointer)*V+(getCaseIndex(unr.caseid).loc[baseselect].startfrom-D[getCaseIndex(unr.caseid).loc[baseselect].disknum].curdiskpointer)
			  ))
			  {
			  	baseselect=j;
			  }
			}
		}
			    /*if((baseselect==-1)||(readstart))
			    {
			     baseselect=j;
				}*/
				if(baseselect>-1)
				{
				rf.iscontinuous=true;
				rf.queryid=unr.taskid;
				rf.caseid=unr.caseid;
				rf.storeid=getCaseIndex(unr.caseid).loc[baseselect].disknum;
				rf.Size=unr.Size;
				rf.querytime=unr.querytime;
				//rd.locid=;
				rf.deltoaccept=0;
				rf.start=getCaseIndex(unr.caseid).loc[baseselect].startfrom;
				rf.readstart=(getCaseIndex(unr.caseid).loc[baseselect].startfrom<D[rf.storeid].curdiskpointer)*V+(getCaseIndex(unr.caseid).loc[baseselect].startfrom-D[rf.storeid].curdiskpointer);
				getDiskIndex(getCaseIndex(unr.caseid).loc[baseselect].disknum).ReadytoRead.push_back(rf);
				beplaced=true;
				}
				//break;//choose one to move in?
			//}
			//consider pieces-if pieces length>1,push into queue,now not need to consider.
		
		getCaseIndex(unr.caseid).selected_read=true;
		//std::cout<<"check "<<unr.caseid<<" "<<unr.Size<<" "<<f(unr.querytime,false)<<" "<<getCaseIndex(unr.caseid).totalreadvalue<<"\n";//use in debug
		}
		if(beplaced||getCaseIndex(unr.caseid).selected_read)
		{
		getCaseIndex(unr.caseid).totalreadvalue+=f(unr.querytime,false)*unr.Size;
	    }
	    else
	    {
	    	getCaseIndex(unr.caseid).unfinishedreadqueryid.push_back(unr.taskid);//unselected
	    	Unusetask[unr.taskid]=true;
		}
		}
		else
		{
			getCaseIndex(unr.caseid).unfinishedreadqueryid.push_back(unr.taskid);//unfinished
	    	Unusetask[unr.taskid]=true;
		}
	}
	}
	//use multi priorityqueues to better equip woth the manager.
	//or use M tags of unread priority_queue?
	//how to allocate the unread task?
    
 
	//allocate reading task to each disk
	//std::vector<future<std::vector<int>>>Fs;
	//std::vector<std::thread>threads;
	//std::thread threadset[10];
	for(int i=0;i<N;i++)//using multithread
	{
	    //threadset[i]=std::thread(readdecision,i,getDiskIndex(i).curdiskpointer);
	    readdecision(i,getDiskIndex(i).curdiskpointer);
	    //threads.emplace_back(readdecision(i,getDiskIndex(i).curdiskpointer));
		//Fs.push_back(async(launch::async,readdecision,(int)i,(long int)getDiskIndex(i].curdiskpointer);
	}
	//for(auto& th:threads){th.join();}
	/*for(int i=0;i<N;i++)
	{
		if(threadset[i].joinable())
		{
			threadset[i].join();
		}
	}*/
	long int cz=0,cd=Qread.size();
//	std::cout<<qreadnum<<"\n";
    contime++;
	for(int i=0;i<qreadnum;i++)
	{
		getCaseIndex(Qread[i].obj).totalreadvalue=0;
		getCaseIndex(Qread[i].obj).selected_read=false;
		//getCaseIndex(Qread[i].obj).readqueryid.clear();
		if(!getCaseIndex(Qread[i].obj).read)
		{
			getCaseIndex(Qread[i].obj).readqueryid.clear();
			if(((getCaseIndex(Qread[i].obj).delayfinishreadtaskid.empty())||
			(getCaseIndex(Qread[i].obj).delayfinishreadtaskid.front()-getCaseIndex(Qread[i].obj).Size-(tagaliveperiod[getCaseIndex(Qread[i].obj).tag]/getCaseIndex(Qread[i].obj).Size)>contime))
			&&(tagaliveperiod[getCaseIndex(Qread[i].obj).tag]+getCaseIndex(Qread[i].obj).addtime-getCaseIndex(Qread[i].obj).Size*getCaseIndex(Qread[i].obj).Size<contime)
			&&(contime>beginusestrategy))
			{
				getCaseIndex(Qread[i].obj).delayfinishreadtaskid.push(Qread[i].ch);
				continue;
			}
			finishread.push_back(Qread[i].ch);
			n_rsp++;
		}
		else
		{
		if(f(Qread[i].querytime,false)*1.0/1000*getCaseIndex(Qread[i].obj).Size>=considernextread)//if valuable to read push into unread case
		{
			unr.taskid=Qread[i].ch;
		    unr.caseid=Qread[i].obj;
		    unr.querytime=Qread[i].querytime;
		    unr.Size=getCaseIndex(Qread[i].obj).Size;
		    unr.deled=false;
		    U.push(unr);
		}
		else
		{
			cs[Qread[i].obj].unfinishedreadqueryid.push_back(Qread[i].ch);
			Unusetask[Qread[i].ch]=true;
		}
		}
	}
	
	std::cout<<n_rsp<<"\n";
	//printf("%ld\n",n_rsp);
	for(long int i=0;i<n_rsp;i++)
	{
		std::cout<<finishread[i]<<"\n";
		//printf("%d\n",finishread[i]);
	}
	finishread.clear();
	//std::cout<<"Contime "<<contime<<"\n";
	/*for(auto& fut:Fs)
	{

	}*/
	//deal with those unread
	fflush(stdout);
  }
  static OBS& Instance()
  {
  	//加入时间这一参数？
  	OBS *parent;
   static OBS obs;
   return obs;
  }
};
OBS::disk OBS::D[10]; 
std::unordered_map<long int,OBS::Case>OBS::cs={};
int OBS::contime=1;
int OBS::tagnotenum[17][10]={0};
int OBS::tagcalcnum[17]={0};
int main()
{
	PreInput();
	OBS& obs=OBS::Instance();
	//obs.contime=0;
	//*OBS.Instance
	obs.Initial();
	for(long int time=1;time<=T+105;time++)//Interact
	{
		std::string TIMESTAMP;
		int timeline;//Interact time line
		std::cin>>TIMESTAMP>>timeline;
		considernextread=2.0+std::max(-1.5,log2(fre_read[0][timeline/1800+(timeline%1800>0)]*1.0/maxperiodreadsize));
		//contime-update in obs.
        //considernextread=fre_read[0][time/1800]*1.0/(30000000.0/T);
		std::cout<<TIMESTAMP<<" "<<timeline<<"\n";
		//printf("%s %d\n",TIMESTAMP,timeline);
	//	std::cout<<"check contime "<<obs.contime<<"\n";
		int delnum,writenum,readnum;
		std::cin>>delnum;
		obs.delInput(delnum);
		std::cin>>writenum;
		obs.writeInput(writenum);
		std::cin>>readnum;
		obs.readInput(readnum);
		//std::cout<<"Total read task score: "<<totalreadtaskscore<<"\n";//debug
	}
	return 0;
}
