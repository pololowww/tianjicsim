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

#ifndef _GLOBALS_HPP_
#define _GLOBALS_HPP_
#include <string>
#include <vector>
#include <iostream>
#include <list>
#include <map>

using namespace std;

/*all declared in main.cpp*/

int GetSimTime();

class Stats;
Stats * GetStats(const std::string & name);

extern bool gPrintActivity;

extern int gK;
extern int gN;
extern int gC;

extern bool sample = true;

extern vector<vector<vector<int>>> nodedes;
extern vector<vector<vector<int>>> nodenum;

extern vector<vector<int>> nodeorder;
extern vector<int> familytime;

extern vector<vector<map<int,int>>> LUT;
extern vector<map<int,int>> LUT_MUL;

extern vector<int> lut_mul;//每个节点多播目的地
extern vector<vector<int>> packet_mul;//每个phase，每个节点发出的包是否多播，多播则为1
extern vector<vector<int>> phase_packet_num;//每个phase，每个节点需要发出的包的数目
extern vector<vector<int>> phase_all_packet_num;//每个phase每个节点与其相关的未完成的包的数目（接受+发送，且在包retire后才--）
extern vector<vector<int>> phase_end_nodes;//每个phase完成全部传输的节点的数量
extern vector<vector<int>> phase_nodes_compute;//每个phase每个节点计算的时间延迟
extern vector<vector<int>> phase_nodes_a;//每个phase每个节点计算的a延迟
extern vector<vector<int>> phase_nodes_s1;//每个phase每个节点计算的s1延迟
extern vector<vector<int>> phase_nodes_s2;//每个phase每个节点计算的s2延迟

extern map<int,int> phase_group;//每个节点属于哪个group，1表示节点编号，2表示group编号
extern vector<vector<int>> group_size;//每个phase，每个group需要进行路由操作的节点的个数
extern vector<int> group_phases;//每个group处于第几phase

extern map<int,int> channel_occupation;
extern vector<vector<int>> step_occupation;//每个测量段每个链路转运的包的数目
extern vector<vector<int>> occupation;//每个链路转运的包的数目
extern vector<vector<int>> node_occupation;//每个测量段每个节点接收的包的数目
extern vector<int> phase_cost;//每个phase功耗
extern map<pair<int,int>,int> channel_connection;
extern map<pair<int,int>,int> outport_connection;
extern std::ofstream ofs_occup;
extern std::ofstream ofs_node;
extern std::ofstream ofs_edge;
extern std::ofstream ofs_group;
extern std::ofstream ofs_phase;
extern std::ofstream ofs_channel;
extern int measure_step;
extern int compute_mode;
extern int core_width;
extern int core_height;
extern int chip_width;
extern int chip_height;
extern int phase_num;
extern int min_phase_num;
extern vector<vector<int>> swapnodes;
extern int phase_time;
extern int NODE_NUM;
extern int chanel_num;
extern vector<int> packet_num;
extern vector<int> packet_cost;
extern vector<int> group_time;
extern vector<vector<int>> arr_temp;
extern vector<vector<int>> arr_temp_double;
extern vector<vector<int>> arr_temp2;
extern std::vector<std::string> files;
extern int group_num;

extern int gNodes;

extern bool gTrace;

extern std::ostream * gWatchOut;

#endif
