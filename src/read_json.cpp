#include "read_json.hpp"
#include <dirent.h>

using namespace std;

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#elif defined(_MSC_VER)
#pragma warning(disable : 4996)
#endif

int width = core_width;
int height = core_height;
int copynum=0;
int idcast(int x, int y,int cx,int cy)
{
	int step=(x + width* y+x*width)%(core_width*core_height);
	int ori=x + width* y;
	return ori + width*height*(cy*chip_width+cx);
} //将坐标映射为编号
int sign(int num1,int num2){
	int x1=num1%width;
	int x2=num2%width;
	int y1=num1/width;
	int y2=num2/width;
	int dx=x1>x2?x1-x2:x2-x1;
	int dy=y1>y2?y1-y2:y2-y1;
	return dx+dy;


}

void GetFileName(std::string path, std::vector<std::string> &files) {
    DIR *pDir;   //  是头文件<dirent.h>的类型
    struct dirent *ptr;  // opendir、readdir这些都是头文件dirent.h
    if (!(pDir = opendir(path.c_str()))) return;
    while ((ptr = readdir(pDir)) != 0) {
        // strcmp是C语言里的，只导入string,然后std::strcmp都是没有的
        if (strcmp(ptr->d_name, ".") != 0 && strcmp(ptr->d_name, "..") != 0) {
            files.push_back(path + "/" + ptr->d_name);  // 可以只保留名字
			cout<<path + "/" + ptr->d_name<<endl;
        }
    }
    closedir(pDir);
}

void file_read(string filepath,int group_num,int phase_num){

	std::ifstream ifs;
	ifs.open(filepath);
	assert(ifs.is_open());
	// std::ofstream ofs;
	// ofs.open("data/json_data");
	// assert(ofs.is_open());
	// std::ofstream ofs_mul;
	// ofs_mul.open("data/json_data_mul");
	// assert(ofs_mul.is_open());
	std::ofstream ofs_group;
	ofs_group.open("data/json_group",ios::out|ios::app);
	assert(ofs_group.is_open());
	int i,j;
	int node_num=width * height*chip_width*chip_height;
    //int arcs[width * height*chip_width*chip_height][width * height*chip_width*chip_height][30] = {0};				  //总体的邻接矩阵，值表示传输几个包
    //int multi_packets[width * height*chip_width*chip_height][width * height*chip_width*chip_height][30] = {0}; //记录多播的包的起始路线，每个phase更新

	set<int> node_group;
	string lineStr;
    vector<vector<string>> strArray;
    i=0;
    char* end;
	if(ifs.fail())
        cout<<"读取文件失败"<<endl;

		cout << "loading.......phase" <<endl;

    while (getline(ifs, lineStr))
    {
        j=0;
        // 打印整行字符串
        // cout << lineStr << endl;
        // 存成二维表结构
        stringstream ss(lineStr);


        string str;
        vector<string> lineArray;

        // 按照逗号分隔
        while (getline(ss, str, ','))
        {

			int var_temp= int(static_cast<int>(strtol(str.c_str(),&end,10)));
			if(var_temp>0){
				packet_num[group_num]+=var_temp;packet_cost[group_num]+=var_temp*sign(i,j);
				// cout<<packet_num<<endl;

			    // cout<<i<<" "<<j<<" "<<sign(i,j)<<endl;
			}
			int core_id=i;
			int dst_id=j;
			// if(core_id/160!=dst_id/160&&var_temp>0)
			// cout<<core_id<<"->"<<dst_id<<" "<<var_temp<<endl;

						if(var_temp>0)
						// {LUT[phase_num][core_id].insert(pair<int,int>(dst_id,var_temp));
						{
							// cout<<"insert"<<i<<" "<<j<<endl;
							node_group.insert(i);node_group.insert(j);
								for(int m=0;m<phase_num;m++){
							nodedes[m][core_id].push_back(dst_id);
							nodenum[m][core_id].push_back(var_temp);

							// if(core_id==7080)cout<<dst_id<<" "<<var_temp<<endl;
						// if(core_id>960&&core_id<1120)
						// {cout<<phase_num<<" "<<atoi(core["core_id"]["core_x"].toStyledString().c_str())<<","<<atoi(core["core_id"]["core_y"].toStyledString().c_str());
						// cout<<"->"<<atoi(core["packets"][packetid]["dst_core_id"]["core_x"].toStyledString().c_str())<<","<<atoi(core["packets"][packetid]["dst_core_id"]["core_y"].toStyledString().c_str())<<endl;}
						phase_all_packet_num[m][core_id]+=var_temp;
						phase_all_packet_num[m][dst_id]+=var_temp;
								}


						// phase_all_packet_num[1][core_id]+=var_temp;
						// phase_all_packet_num[1][dst_id]+=var_temp;
						// cout<<core_id<<"packets"<<phase_all_packet_num[phase_num][core_id]<<endl;
						// cout<<dst_id<<"packets"<<phase_all_packet_num[phase_num][dst_id]<<endl;
						}
		        //string -> int
            j++;
        }
		// cout<<numm<<endl;
		// cout<<nodedes[i][0]<<endl;

		// cout<<endl<<i;
        i++;
   //     strArray.push_back(lineArray);
    }


		// cout<<node_group[0].size()<<endl;


set<int>::iterator iter;
		for(iter=node_group.begin();iter!=node_group.end();iter++){
			ofs_group<<*iter<<" ";
		}
		ofs_group<<endl;




	// for (int i = 0; i < width * height; i++)
	// {
	// 	for (int j = 0; j < 100; j++)
	// 	{
	// 		cout<<multi_arcs[i][j]<<" ";
	// 	}
	// 	cout<<endl;
	// }
	// 	for (int i = 0; i < height*width; ++i)
	// 	{
	// 		for (int j = 0; j < height*width; ++j)
	// 		{
	// 			if (arcs[i][j] != 0) { cout << i + 1 << "->" << j + 1 << "=" << arcs[i][j] << endl; }
	// printf("%-2d\x20", arcs[i][j]);
	// }
	// printf("\n");
	// }//输出arcs矩阵
    getchar();
}


void read_csv(BookSimConfig const & config){
	// packet_cost=0;
	// packet_num=0;
	string filepath=config.GetStr("csv_filepath");
	GetFileName(filepath,files);
	group_num=files.size();
	cout<<"共加载了"<<group_num<<"个任务"<<endl;

		width=config.GetInt("core_x");
	height=config.GetInt("core_y");
	chip_width=config.GetInt("chip_x");
	chip_height=config.GetInt("chip_y");
	int phase_num=config.GetInt("total_phase");

	std::ofstream ofs_group;
	ofs_group.open("data/json_group");
	assert(ofs_group.is_open());
	ofs_group.close();
	for(int i=0;i<files.size();i++)
	{
		file_read(files[i],i,phase_num);
		}



}

void read_json( BookSimConfig const & config)
{
	width=config.GetInt("core_x");
	height=config.GetInt("core_y");
	chip_width=config.GetInt("chip_x");
	chip_height=config.GetInt("chip_y");
	string filepath=config.GetStr( "json_filepath" );
	std::ifstream ifs;
	ifs.open(filepath);
	assert(ifs.is_open());


	std::ofstream ofs;
	ofs.open("data/json_data");
	assert(ofs.is_open());
	std::ofstream ofs_mul;
	ofs_mul.open("data/json_data_mul");
	assert(ofs_mul.is_open());
	std::ofstream ofs_group;
	ofs_group.open("data/json_group");
	assert(ofs_group.is_open());

	int ***arcs;


	int i,j;

	int node_num=width * height*chip_width*chip_height;

	arcs=(int***)malloc(node_num*sizeof(int**));

	for(i=0;i<node_num;i++)
	{
		arcs[i]=(int**)malloc(node_num*sizeof(int*));
		for(j=0;j<node_num;j++)
		{arcs[i][j]=(int*)malloc(sizeof(int));}

	}

	int ***multi_packets;

	multi_packets=(int***)malloc(node_num*sizeof(int**));
	for(i=0;i<node_num;i++)
	{
		multi_packets[i]=(int**)malloc(node_num*sizeof(int*));

		for(j=0;j<node_num;j++)
		{multi_packets[i][j]=(int*)malloc(sizeof(int));}

	}


    //int arcs[width * height*chip_width*chip_height][width * height*chip_width*chip_height][30] = {0};				  //总体的邻接矩阵，值表示传输几个包
    //int multi_packets[width * height*chip_width*chip_height][width * height*chip_width*chip_height][30] = {0}; //记录多播的包的起始路线，每个phase更新
    int multi_arcs[node_num][30] = {0};					  //多播矩阵，第一个下标表示编号为几的核，第二个下标表示第几个phase，值表示其目的地


	Json::Reader reader;
	Json::Value root;
	// 解析到root，root将包含Json里所有子元素
	if (!reader.parse(ifs, root, false))
	{
		cerr << "parse failed \n";
		return;
	}

	for (int i = 0; i < node_num; i++)
	{
		for (int j = 0; j <30; j++)
		{
			multi_arcs[i][j]=-1;
		}
	}
	int phase_num = 0;
	vector<set<int>> node_group(40);

	while (root["phase_list"][to_string(phase_num)].toStyledString().length() > 5)
	{
		// cout << "loading.......phase" << phase_num << endl;

		int group_num = 0;
		Json::Value phaseObj = root["phase_list"][to_string(phase_num)];

		while (phaseObj[to_string(group_num)].size() > 0)
		{
			int core_num = phaseObj[to_string(group_num)].size();

			for (int coreid = 0; coreid < core_num; coreid++)
			{  //遍历一遍，储存多播矩阵
				Json::Value core = phaseObj[to_string(group_num)][coreid];

				int multicore_id = idcast(atoi(core["core_id"]["core_x"].toStyledString().c_str()), atoi(core["core_id"]["core_y"].toStyledString().c_str()),atoi(core["core_id"]["chip_x"].toStyledString().c_str()), atoi(core["core_id"]["chip_y"].toStyledString().c_str())); //存储当前core的编号

				if (core["multicast_core"] == true||atoi(core["multicast_core"].toStyledString().c_str())==1)																				   //是多播核
				{
					int multidst_id = idcast(atoi(core["multicast_dst"]["core_x"].toStyledString().c_str()), atoi(core["multicast_dst"]["core_y"].toStyledString().c_str()),atoi(core["multicast_dst"]["chip_x"].toStyledString().c_str()), atoi(core["multicast_dst"]["chip_y"].toStyledString().c_str()));
					multi_arcs[multicore_id][phase_num] = multidst_id;
				//	cout << multicore_id << "可以多播->" << multidst_id<<endl;
				}
			}

			for (int coreid = 0; coreid < core_num; coreid++)
			{
				Json::Value core = phaseObj[to_string(group_num)][coreid];
				int core_id = idcast(atoi(core["core_id"]["core_x"].toStyledString().c_str()), atoi(core["core_id"]["core_y"].toStyledString().c_str()),atoi(core["core_id"]["chip_x"].toStyledString().c_str()), atoi(core["core_id"]["chip_y"].toStyledString().c_str())); //存储当前core的编号
				phase_nodes_a[phase_num][core_id]+=atoi(core["A_time"].toStyledString().c_str());
				phase_nodes_s1[phase_num][core_id]+=atoi(core["S1_time"].toStyledString().c_str());
				phase_nodes_s2[phase_num][core_id]+=atoi(core["S2_time"].toStyledString().c_str());
				node_group[group_num].insert(core_id);
				if(copynum==1)
				{node_group[group_num].insert(core_id+320);}
				int packets_num = core["packets"].size();
				for (int packetid = 0; packetid < packets_num; packetid++)
				{
					int dst_id = idcast(atoi(core["packets"][packetid]["dst_core_id"]["core_x"].toStyledString().c_str()), atoi(core["packets"][packetid]["dst_core_id"]["core_y"].toStyledString().c_str()),atoi(core["packets"][packetid]["dst_core_id"]["chip_x"].toStyledString().c_str()), atoi(core["packets"][packetid]["dst_core_id"]["chip_y"].toStyledString().c_str()));
					if (core["packets"][packetid]["multicast_pack"] == false) //单播，直接更改arcs矩阵即可
					{
						int var_temp= atoi(core["packets"][packetid]["pack_num"].toStyledString().c_str());

						arcs[core_id][dst_id][phase_num] += var_temp;
						map<int,int>::iterator cur;
						cur=LUT[phase_num][core_id].find(dst_id+1);
						if(cur==LUT[phase_num][core_id].end())
						{LUT[phase_num][core_id].insert(pair<int,int>(dst_id+1,var_temp));
						// if(core_id>960&&core_id<1120)
						// {cout<<phase_num<<" "<<atoi(core["core_id"]["core_x"].toStyledString().c_str())<<","<<atoi(core["core_id"]["core_y"].toStyledString().c_str());
						// cout<<"->"<<atoi(core["packets"][packetid]["dst_core_id"]["core_x"].toStyledString().c_str())<<","<<atoi(core["packets"][packetid]["dst_core_id"]["core_y"].toStyledString().c_str())<<endl;}
						phase_all_packet_num[phase_num][core_id]+=var_temp;
						phase_all_packet_num[phase_num][dst_id]+=var_temp;
						}
						if(copynum==1){
						cur=LUT[phase_num][core_id+320].find(dst_id+321);
						if(cur==LUT[phase_num][core_id+320].end())
						{LUT[phase_num][core_id+320].insert(pair<int,int>(dst_id+321,var_temp));
						phase_all_packet_num[phase_num][core_id+320]+=var_temp;
						phase_all_packet_num[phase_num][dst_id+320]+=var_temp;
						}}

					}
					else
					{ //是多播包
					int var_temp= atoi(core["packets"][packetid]["pack_num"].toStyledString().c_str());
					if(core_id!=dst_id)
					{LUT_MUL[phase_num].insert(pair<int,int>(core_id,dst_id));
					if(copynum==1){
						LUT_MUL[phase_num].insert(pair<int,int>(core_id+320,dst_id+320));
					}
						arcs[core_id][dst_id][phase_num] += var_temp;
						map<int,int>::iterator cur;
						cur=LUT[phase_num][core_id].find(dst_id+1);
						if(cur==LUT[phase_num][core_id].end())
						{LUT[phase_num][core_id].insert(pair<int,int>(dst_id+1,var_temp));
						phase_all_packet_num[phase_num][core_id]+=var_temp;
						phase_all_packet_num[phase_num][dst_id]+=var_temp;
						}
						if(copynum==1){
							cur=LUT[phase_num][core_id].find(dst_id+321);
						if(cur==LUT[phase_num][core_id].end())
						{LUT[phase_num][core_id+320].insert(pair<int,int>(dst_id+321,var_temp));
						phase_all_packet_num[phase_num][core_id+320]+=var_temp;
						phase_all_packet_num[phase_num][dst_id+320]+=var_temp;
						}
						}
						}



						if (multi_arcs[dst_id][phase_num] > -1)
						{
							multi_packets[core_id][dst_id][phase_num] =1;
						//	cout << core_id << "->" << dst_id << "多播" << core["packets"][packetid]["pack_num"].toStyledString() << endl;
						while (multi_arcs[dst_id][phase_num] > -1)			   //目标节点是多播节点
						{
					//		arcs[dst_id][multi_arcs[dst_id][phase_num]] += atoi(core["packets"][packetid]["pack_num"].toStyledString().c_str());
						    // cout << dst_id << "->" << multi_arcs[dst_id][phase_num] << "多播" << core["packets"][packetid]["pack_num"].toStyledString() << endl;
							dst_id = multi_arcs[dst_id][phase_num]; //更新目标节点
						}
						} //跟随多播矩阵走完一个多播周期
					}
				}
			}

			group_num++;
		}

		phase_num++;
	}

set<int>::iterator iter;
	for(int i=0;i<27;i++){
		for(iter=node_group[i].begin();iter!=node_group[i].end();iter++){
			ofs_group<<*iter<<" ";
		}
		ofs_group<<endl;
	}

	int phase_packet[node_num]={0};
	int _phase_packet_num[node_num]={0};
	int phase_packet_mul[node_num]={0};


	for(int phase=0;phase<phase_num;phase++){

		for(int t=0;t<node_num;t++)
		{
			phase_packet[t]={0};
		    _phase_packet_num[t]={0};

		}

		for (int i = 0; i < node_num; i++)
	{
		for (int j = 0; j <  node_num; j++)
		{
				if((multi_packets)[i][j][phase]==1){
					phase_packet_mul[i]=1;

				}
			if(arcs[i][j][phase]>0)
			{
				phase_packet[i]=j+1;
				_phase_packet_num[i]=arcs[i][j][phase];
				if(_phase_packet_num[i]>10000){phase_packet[i]=0;_phase_packet_num[i]=0;}

			}
		}
	}//遍历一遍

		for (int i = 0; i < node_num; i++)
	{

		if(_phase_packet_num[i]>0)
		{

			ofs<<phase_packet[i]<<" ";
	//	    phase_packet_num[i]--;
		}
		else {ofs<<0<<" ";}
		if(phase_packet_mul[i]==1){ofs_mul<<1<<" ";}
		else{ofs_mul<<0<<" ";}
	}
	ofs<<endl;
	for (int i = 0; i < node_num; i++)
	{
			ofs<<_phase_packet_num[i]<<" ";
	}
	ofs<<endl;
	ofs_mul<<endl;
	}


	for (int i = 0; i < node_num; i++)
	{
			ofs<<multi_arcs[i][0]<<" ";

	}
	ofs.close();

	for(i=0;i<node_num;i++)
	{
	for(j=0;j<node_num;j++)
	{
	free(arcs[i][j]);

	}
	free(arcs[i]);
	}
	free(arcs);

	for(i=0;i<node_num;i++)
	{
	for(j=0;j<node_num;j++)
	{
	free(multi_packets[i][j]);

	}
	free(multi_packets[i]);
	}
	free(multi_packets);

	// for (int i = 0; i < width * height; i++)
	// {
	// 	for (int j = 0; j < 100; j++)
	// 	{
	// 		cout<<multi_arcs[i][j]<<" ";
	// 	}
	// 	cout<<endl;
	// }
	// 	for (int i = 0; i < height*width; ++i)
	// 	{
	// 		for (int j = 0; j < height*width; ++j)
	// 		{
	// 			if (arcs[i][j] != 0) { cout << i + 1 << "->" << j + 1 << "=" << arcs[i][j] << endl; }
	// printf("%-2d\x20", arcs[i][j]);
	// }
	// printf("\n");
	// }//输出arcs矩阵

} //读取json文件，并存储在矩阵里
