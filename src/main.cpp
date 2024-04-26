// $Id$

/*
 Copyright (c) 2007-2015, Trustees of The Leland Stanford Junior University
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.
 Redistributions in binary form must reproduce the above copyright notice, this
 list of conditions and the following disclaimer in the documentation and/or
 other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*main.cpp
 *
 *The starting point of the network simulator
 *-Include all network header files
 *-initilize the network
 *-initialize the traffic manager and set it to run
 *
 *
 */
#include <sys/time.h>

#include <string>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <numeric>
#include<algorithm>

#include <sstream>
#include "booksim.hpp"
#include "routefunc.hpp"
#include "traffic.hpp"
#include "booksim_config.hpp"
#include "trafficmanager.hpp"
#include "random_utils.hpp"
#include "network.hpp"
#include "injection.hpp"
#include "power_module.hpp"
#include "anynet_file_create.hpp"
#include "read_json.hpp"

using namespace std;

///////////////////////////////////////////////////////////////////////////////
//Global declarations
//////////////////////

 /* the current traffic manager instance */
vector<int> group_phases(27,0);
TrafficManager * trafficManager = NULL;

int GetSimTime() {
  return trafficManager->getTime();
}

map<int,int> channel_occupation;
map<pair<int,int>,int> channel_connection;
map<pair<int,int>,int> outport_connection;
vector<vector<int>> step_occupation(5000,vector<int>(6180,0));
vector<vector<int>> occupation(40,vector<int>(6180,0));
vector<vector<int>> arr_temp2(40,vector<int>(6180,0));
vector<vector<int>> node_occupation(5000,vector<int>(8242,0));


class Stats;
Stats * GetStats(const std::string & name) {
  Stats* test =  trafficManager->getStats(name);
  if(test == 0){
    cout<<"warning statistics "<<name<<" not found"<<endl;
  }
  return test;
}

/* printing activity factor*/
bool gPrintActivity;

int gK;//radix
int gN;//dimension
int gC;//concentration

int core_width;
int core_height;
int chip_width;
int chip_height;
int measure_step;
int compute_mode;
int chanel_num;
int group_num;
int min_phase_num=0;
int NODE_NUM;
int phase_time=0;
std::vector<std::string> files;
vector<int> packet_num(40);
vector<int> packet_cost(40);
vector<int> group_time(40);
std::ofstream ofs_occup;
std::ofstream ofs_node;
std::ofstream ofs_channel;
std::ofstream ofs_edge;
std::ofstream ofs_group;
std::ofstream ofs_phase;

vector<int> familytime(4,0);
vector<vector<int>> nodeorder(8,vector<int>(1600,0));
vector<vector<int>> arr_temp(40,vector<int>(16000,0));
vector<vector<int>> arr_temp_double(16000, vector<int>(3,0));
vector<vector<vector<int>>> nodedes(1000,vector<vector<int>>(16000));
vector<vector<vector<int>>> nodenum(1000,vector<vector<int>>(16000));
vector<vector<map<int,int>>> LUT(27, vector<map<int,int>>(16000));
vector<map<int,int>> LUT_MUL(16000);
vector<int> lut_mul(1,0);
map<int,int> phase_group;
vector<vector<int>> swapnodes;
vector<vector<int>> group_size(27, vector<int>(27,0));
vector<vector<int>> phase_packet_num(27, vector<int>(16000,0));
vector<vector<int>> phase_all_packet_num(1000, vector<int>(16000,0));
vector<vector<int>> phase_end_nodes(27, vector<int>(27,0));
vector<vector<int>> packet_mul(27, vector<int>(16000,0));
vector<int> phase_cost(27,0);
vector<vector<int>> phase_nodes_compute(54, vector<int>(16000,0));
vector<vector<int>> phase_nodes_a(27, vector<int>(16000,0));
vector<vector<int>> phase_nodes_s1(27, vector<int>(16000,0));
vector<vector<int>> phase_nodes_s2(27, vector<int>(16000,0));
int gNodes;

//generate nocviewer trace
bool gTrace;

ostream * gWatchOut;



void load_lut(vector<vector<int>>& lut, BookSimConfig const & config)
{
	const char* fname = "./data/json_data";
  const char* fname_mul = "./data/json_data_mul";
  const char* fname_compute = "./data/json_compute";
  const char* fname_group = "./data/json_group";

  int phases_num=config.GetInt("total_phase");
	int NODE_NUM = config.GetInt("core_x")*config.GetInt("core_y")*config.GetInt("chip_x")*config.GetInt("chip_y");


	ifstream in(fname);
  ifstream in_mul(fname_mul);
  ifstream in_compute(fname_compute);
  ifstream in_group(fname_group);

	if(!in) {
        cerr << fname << endl;
		cerr << "Invalid LUT filename." << endl;
		abort();
	}

  if(!in_mul) {
        cerr << fname_mul << endl;
		cerr << "Invalid LUT_MUL filename." << endl;
		abort();
	}

  if(!in_compute) {
        cerr << fname_compute << endl;
		cerr << "Invalid LUT_COMPUTE filename." << endl;
		abort();
	}

  if(!in_group) {
        cerr << fname_group << endl;
		cerr << "Invalid LUT_GROUP filename." << endl;
		abort();
	}

  	for (int i=0;i<phases_num;i++) {
		string line;
		getline(in_mul,line);
		istringstream line_in(line);
		for (int j=0;j<NODE_NUM;j++) {
			double temp;
			line_in >> temp;
			if(bool(line_in)) {
        packet_mul[i][j] = static_cast<int>(temp);
  //      if(j==NODE_NUM-1)cout<<endl;
			}
		}
	}

	for (int i=0;i<phases_num*2;i++) {
		string line;
		getline(in,line);
		istringstream line_in(line);
	// 	for (int j=0;j<NODE_NUM;j++) {
	// 		double temp;
	// 		line_in >> temp;
	// 		if(bool(line_in)) {
  //       if(i%2==0)
	// 			{//lut[i/2][j] = static_cast<int>(temp);
  //       }
  //       else{phase_packet_num[i/2][j]=static_cast<int>(temp);
  //       phase_all_packet_num[i/2][j]+=static_cast<int>(temp);
  //       if(LUT[i/2][j].size()>0)
  //       phase_all_packet_num[i/2][LUT[i/2][j]-1]+=static_cast<int>(temp);

  //      }

  // //      if(j==NODE_NUM-1)cout<<endl;
	// 		}
	// 	}
	}
  // for(int i=0;i<phases_num;i++){
  //   for (int j=0;j<NODE_NUM;j++) {cout<<phase_all_packet_num[i][j]<<" ";}
  //   cout<<endl;

  // }
  string line;
		getline(in,line);
		istringstream line_in(line);
		for (int j=0;j<NODE_NUM;j++) {
			double temp;
			line_in >> temp;
			if(bool(line_in)) {
				lut_mul[j] = static_cast<int>(temp);
       // cout<<lut_mul[j]<<" ";

			}
		}



  for (int i=0;i<phases_num*2;i++) {
		string line;
		getline(in_compute,line);
		istringstream line_in(line);
		for (int j=0;j<NODE_NUM;j++) {
			double temp;
			line_in >> temp;
			if(bool(line_in)) {
				phase_nodes_compute[i][j] = static_cast<int>(temp);
  //      if(j==NODE_NUM-1)cout<<endl;
			}
		}
	}

  for (int i=0;i<files.size();i++) {
		string line;
		getline(in_group,line);
		istringstream line_in(line);
		for (int j=0;j<NODE_NUM;j++) {
			double temp;
			line_in >> temp;
			if(bool(line_in)) {
        map<int ,int > ::iterator current_group;
        current_group = phase_group.find(temp);
				if(current_group!=phase_group.end()){phase_group.erase(current_group);}
        phase_group.insert(pair<int,int>(temp,i));

  //      if(j==NODE_NUM-1)cout<<endl;
			}
		}
	}
  ofs_group<<"const nodegroup = new Array();"<<endl;
  for (int j=0;j<NODE_NUM;j++) {
  map<int ,int > ::iterator current_group;
        current_group = phase_group.find(j);
				if(current_group!=phase_group.end()){
          // cout<<j<<" "<<current_group->second<<endl;
          ofs_group<<"nodegroup["<<j<<"]="<<current_group->second<<";"<<endl;
          }
  }//modifyyy
  ofs_group<<"export default nodegroup;"<<endl;
}
bool Simulate( BookSimConfig const & config,vector<vector<int>>& LUT )
{
  vector<Network *> net;

  load_lut(LUT,config);
  int subnets = config.GetInt("subnets");
  /*To include a new network, must register the network here
   *add an else if statement with the name of the network
   */
  net.resize(subnets);
  for (int i = 0; i < subnets; ++i) {
    ostringstream name;
    name << "network_" << i;
    net[i] = Network::New( config, name.str() );
  }

  /*tcc and characterize are legacy
   *not sure how to use them
   */

  // assert(trafficManager == NULL);

  trafficManager = TrafficManager::New( config, LUT,net) ;

  /*Start the simulation run
   */

  double total_time; /* Amount of time we've run */
  struct timeval start_time, end_time; /* Time before/after user code */
  total_time = 0.0;
  gettimeofday(&start_time, NULL);

  bool result = trafficManager->Run() ;


  gettimeofday(&end_time, NULL);
  total_time = ((double)(end_time.tv_sec) + (double)(end_time.tv_usec)/1000000.0)
            - ((double)(start_time.tv_sec) + (double)(start_time.tv_usec)/1000000.0);
export_data(GetSimTime());
map<pair<int,int>,int>::iterator iter;
int num=core_width*core_height*chip_width*chip_height;
int cha_num=0;
int sum1,mean1,sum2,mean2;
vector<int> occ_num(40,0);
vector<int> current_double(40,0);
for(int i=0;i<2;i++)
{
  vector<vector<int>> arr_temp_double(16000, vector<int>(3,0));
 for(iter = channel_connection.begin();iter!=channel_connection.end(); iter++){
   if(iter->first.first<num&&iter->first.second<num)
  { ofs_channel<<iter->first.first<<" "<<iter->first.second<<" "<<occupation[i][iter->second]<<endl;
    if(i==0)cha_num++;
    if(occupation[i][iter->second]>0){
      arr_temp[i][occ_num[i]]=int(occupation[i][iter->second]);
      occ_num[i]++;

      int small=iter->first.first<iter->first.second?iter->first.first:iter->first.second;
      int big=iter->first.first>iter->first.second?iter->first.first:iter->first.second;
      bool cflag=false;
      for(int i=0;i<arr_temp_double.size();i++)
      {
        if(arr_temp_double[i][0]==small&&arr_temp_double[i][1]==big){
          arr_temp_double[i][2]+=occupation[i][iter->second];
          cflag=true;
          break;
        }
      }
      if(cflag==false){
        arr_temp_double[current_double[i]]={small,big,occupation[i][iter->second]};
      current_double[i]++;
      }
    }
  }
 }
 arr_temp[i].resize(occ_num[i]);
  arr_temp_double.resize(current_double[i]);
  arr_temp2[i].resize(current_double[i],0);

        for(int s=0;s<current_double[i];s++)
      {
        // cout<<arr_temp_double[0][0]<<"-"<<arr_temp_double[0][1]<<endl;
        // cout<<arr_temp_double[1][0]<<"-"<<arr_temp_double[1][1]<<endl;
        arr_temp2[i][s]=arr_temp_double[s][2];
        // cout<< arr_temp2[i][s]<<endl;
      }

 sort(arr_temp[i].begin(),arr_temp[i].end());
 sort(arr_temp2[i].begin(),arr_temp2[i].end());
 sum1 = accumulate(arr_temp[i].begin(), arr_temp[i].end(), 0);   // accumulate函数就是求vector和的函数；
 mean1=sum1/ cha_num;
 sum2 = accumulate(arr_temp2[i].begin(), arr_temp2[i].end(), 0);   // accumulate函数就是求vector和的函数；
 mean2=sum2 / cha_num*2;


}

// cout<<"======= "<<core_width<<"*"<<core_height<<"Network Statistics ======="<<endl;
// cout<<"Total run time "<<total_time<<endl;
// for(int i=0;i<group_num;i++)
// {
//   cout<<"-------------任务"<<i+1<<"---------------"<<endl;
//   cout<<"数据路径： "<<files[i]<<endl;
// cout<<"Network cost="<<packet_cost[i]<<endl;
// cout<<"Packet num="<<packet_num[i]<<endl;
// cout<<"Total phase time "<<group_time[i]<<endl;
// cout<<endl;
// cout<<"保留双向链路：";
// cout<<"Channels num="<<cha_num<<endl;
// cout<<"Channels occupated="<<occ_num[i]<<endl;
// cout<<"Max occupation="<<arr_temp[i][occ_num[i]-1]<<endl;
// cout<<"Average occupation="<<mean1<<endl;
// cout<<"   ====箱线图===="<<endl;
// cout<<"     --上边缘--     "<<arr_temp[i][occ_num[i]-1]<<endl;
// cout<<"   --上四分位数--   "<<arr_temp[i][int(0.75*occ_num[i]-1)]<<endl;
// cout<<"     --中位数--     "<<arr_temp[i][int(0.5*occ_num[i]-1)]<<endl;
// cout<<"   --下四分位数--   "<<arr_temp[i][int(0.25*occ_num[i]-1)]<<endl;
// cout<<"     --下边缘--     "<<arr_temp[i][0]<<endl;
// cout<<endl;
// cout<<"双向链路合并：";
// cout<<"Channels num="<<cha_num/2<<endl;
// cout<<"Channels occupated="<<current_double[i]<<endl;
// cout<<"Max occupation="<<arr_temp2[i][current_double[i]-1]<<endl;
// cout<<"Average occupation="<<mean2<<endl;
// cout<<"   ====箱线图===="<<endl;
// cout<<"     --上边缘--     "<<arr_temp2[i][current_double[i]-1]<<endl;
// cout<<"   --上四分位数--   "<<arr_temp2[i][int(0.75*current_double[i]-1)]<<endl;
// cout<<"     --中位数--     "<<arr_temp2[i][int(0.5*current_double[i]-1)]<<endl;
// cout<<"   --下四分位数--   "<<arr_temp2[i][int(0.25*current_double[i]-1)]<<endl;
// cout<<"     --下边缘--     "<<arr_temp2[i][0]<<endl;
// cout<<endl;
// }
//   if(!sample)
//  { for (int i=0; i<subnets; ++i) {

//     ///Power analysis
//     if(config.GetInt("sim_power") > 0){
//       Power_Module pnet(net[i], config);
//       pnet.run();
//     }

//     delete net[i];
//   }
//  }

  delete trafficManager;
  // trafficManager = NULL;

  return result;

}


int main( int argc, char **argv )
{
	ofs_occup.open("data/data_occup");
	assert(ofs_occup.is_open());
  ofs_node.open("data/node_occup");
	assert(ofs_node.is_open());
  ofs_edge.open("data/edge_occup");
	assert(ofs_edge.is_open());
  ofs_group.open("data/node_group");
	assert(ofs_group.is_open());
  ofs_phase.open("data/group_phase");
	assert(ofs_phase.is_open());
  ofs_channel.open("data/channel_occ");
	assert(ofs_channel.is_open());


  BookSimConfig config;


  if ( !ParseArgs( &config, argc, argv ) ) {
    cerr << "Usage: " << argv[0] << " configfile... [param=value...]" << endl;
    return 0;
 }
  core_width=config.GetInt("core_x");
  core_height=config.GetInt("core_y");
  chip_width=config.GetInt("chip_x");
  chip_height=config.GetInt("chip_y");
  bool sample = true;
   if(sample)
 {read_csv(config);}
 else{read_json(config);}
//  cout<<"111111"<<endl;

  int node_nums=readanynet(config);
  /*initialize routing, traffic, injection functions
   */
  InitializeRoutingMap( config );



  measure_step=config.GetInt("measure_step");
  compute_mode=config.GetInt("compute_mode");

  int phase_num=config.GetInt("total_phase");


  gPrintActivity = (config.GetInt("print_activity") > 0);
  gTrace = (config.GetInt("viewer_trace") > 0);

  string watch_out_file = config.GetStr( "watch_out" );
  if(watch_out_file == "") {
    gWatchOut = NULL;
  } else if(watch_out_file == "-") {
    gWatchOut = &cout;
  } else {
    gWatchOut = new ofstream(watch_out_file.c_str());
  }



  /*configure and run the simulator
   */
  phase_cost.resize(phase_num,0);
  group_size.resize(phase_num, vector<int>(files.size(),0));
  phase_packet_num.resize(phase_num, vector<int>(node_nums,0));
  phase_all_packet_num.resize(phase_num, vector<int>(node_nums,0));
  phase_end_nodes.resize(phase_num, vector<int>(files.size(),0));
  packet_mul.resize(phase_num, vector<int>(node_nums,0));
  phase_nodes_compute.resize(phase_num*2, vector<int>(node_nums,0));
  phase_nodes_a.resize(phase_num, vector<int>(node_nums,0));
  phase_nodes_s1.resize(phase_num, vector<int>(node_nums,0));
  phase_nodes_s2.resize(phase_num, vector<int>(node_nums,0));
  LUT.resize(phase_num, vector<map<int,int>>(node_nums));
  lut_mul.resize(node_nums,0);
  vector<vector<int>> fake_LUT(phase_num, vector<int>(node_nums,0));

  bool result = Simulate( config,fake_LUT);
  return result ? -1 : 0;
}
