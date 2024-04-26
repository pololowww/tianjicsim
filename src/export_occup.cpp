#include "export_occup.hpp"
using namespace std;

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#elif defined(_MSC_VER)
#pragma warning(disable : 4996)
#endif
void export_data(int _time){

    int dx=core_width;int dy =core_height;
    int xx=chip_width;int yy=chip_height;
    if(_time/measure_step==0)
    	{
            ofs_node<<"export const nodeweights = new Array();  "<<endl;       //先声明一维
            ofs_node<<"for(var i=0;i<24;i++){     //一维长度为5"<<endl;
            ofs_node<<"nodeweights[i]=new Array(); }   //在声明二维"<<endl;
            ofs_edge<<"export const edgeweights = new Array();  "<<endl;       //先声明一维
            ofs_edge<<"for(var i=0;i<24;i++){     //一维长度为5"<<endl;
            ofs_edge<<"edgeweights[i]=new Array(); }   //在声明二维"<<endl;
            ofs_phase<<"export const groupphase = new Array();  "<<endl;       //先声明一维
            ofs_phase<<"for(var i=0;i<24;i++){     //一维长度为5"<<endl;
            ofs_phase<<"groupphase[i]=new Array(); }   //在声明二维"<<endl;
        }


            for(int j=0;j<dx*dy*xx*yy;j++){
                ofs_node<<"nodeweights["<<_time/measure_step<<"]["<<j<<"]="<<node_occupation[_time/measure_step][j]<<";"<<endl;
            }
            for(int j=0;j<8242;j++){
                ofs_edge<<"edgeweights["<<_time/measure_step<<"]["<<j<<"]="<<step_occupation[_time/measure_step][j]<<";"<<endl;
            }


    if(_time/measure_step==0){
        map<pair<int,int>,int>::iterator current_chanel;

        ofs_occup << "const data={" << endl;
            ofs_occup << "nodes: [" << endl;
        for (int jj = 0; jj < yy; jj++)
			{
				for (int ii = 0; ii < xx ; ii++) {

			for (int j = 0; j < dy; j++)
			{
				for (int i = 0; i < dx ; i++) {
					ofs_occup << "{" << endl;
					ofs_occup << "id: 'node-"<<i+j*dx+(jj*xx+ii)*dx*dy<<"'," << endl;
					ofs_occup << "x: "<<30*i+520*ii<<"," << endl;
					ofs_occup << "y: " << 30 * j +340*jj<< "," << endl;
					ofs_occup << "}," << endl;
				}

			}
    }
}
			ofs_occup << "]," << endl;


			ofs_occup << "edges"<<": [" << endl;

        for (int jj = 0; jj < yy; jj++)
			{
				for (int ii = 0; ii < xx ; ii++) {

			for (int j = 0; j < dy - 1; j++)
			{
				for (int i = 0; i < dx - 1; i++) {
					ofs_occup << "{" << endl;
					ofs_occup << "source: 'node-" << i + j * dx +(ii+jj*xx)*dx*dy<< "'," << endl;
					ofs_occup << "target: 'node-" << i + j * dx+1 +(ii+jj*xx)*dx*dy << "'," << endl;
                    current_chanel=channel_connection.find(pair<int,int>(i + j * dx+(ii+jj*xx)*dx*dy,i + j * dx+1+(ii+jj*xx)*dx*dy));
                    ofs_occup << "id: 'edge-" << current_chanel->second << "'," << endl;
                   ofs_occup << "style:{keyshape: {stroke: 'rgba("<<float(step_occupation[0][current_chanel->second]*255/measure_step)<<", 0, "<<255-float(step_occupation[0][current_chanel->second]*255/measure_step)<<", 1)', },}, " << endl;
					ofs_occup << "}," << endl;
					ofs_occup << "{" << endl;
					ofs_occup << "source: 'node-" << i + j * dx +1+(ii+jj*xx)*dx*dy<< "'," << endl;
					ofs_occup << "target: 'node-" << i + j * dx +(ii+jj*xx)*dx*dy << "'," << endl;
                    current_chanel=channel_connection.find(pair<int,int>(i + j * dx+1+(ii+jj*xx)*dx*dy,i + j * dx+(ii+jj*xx)*dx*dy));
                    ofs_occup << "id: 'edge-" << current_chanel->second << "'," << endl;
                    ofs_occup << "style:{keyshape: {stroke: 'rgba("<<float(step_occupation[0][current_chanel->second]*255/measure_step)<<", 0, "<<255-float(step_occupation[0][current_chanel->second]*255/measure_step)<<", 1)', },}, " << endl;
					ofs_occup << "}," << endl;
					ofs_occup << "{" << endl;
					ofs_occup << "source: 'node-" << i + j * dx +(ii+jj*xx)*dx*dy<< "'," << endl;
					ofs_occup << "target: 'node-" << i + j * dx+1+dx +(ii+jj*xx)*dx*dy << "'," << endl;
                    current_chanel=channel_connection.find(pair<int,int>(i + j * dx+(ii+jj*xx)*dx*dy,i + j * dx+1+dx+(ii+jj*xx)*dx*dy));
                    ofs_occup << "id: 'edge-" << current_chanel->second << "'," << endl;
                   ofs_occup << "style:{keyshape: {stroke: 'rgba("<<float(step_occupation[0][current_chanel->second]*255/measure_step)<<", 0, "<<255-float(step_occupation[0][current_chanel->second]*255/measure_step)<<", 1)', },}, " << endl;
					ofs_occup << "}," << endl;
					ofs_occup << "{" << endl;
					ofs_occup << "source: 'node-" << i + j * dx +dx+1+(ii+jj*xx)*dx*dy<< "'," << endl;
					ofs_occup << "target: 'node-" << i + j * dx +(ii+jj*xx)*dx*dy << "'," << endl;
                    current_chanel=channel_connection.find(pair<int,int>(i + j * dx+dx+1+(ii+jj*xx)*dx*dy,i + j * dx+(ii+jj*xx)*dx*dy));
                    ofs_occup << "id: 'edge-" << current_chanel->second << "'," << endl;
                    ofs_occup << "style:{keyshape: {stroke: 'rgba("<<float(step_occupation[0][current_chanel->second]*255/measure_step)<<", 0, "<<255-float(step_occupation[0][current_chanel->second]*255/measure_step)<<", 1)', },}, " << endl;
					ofs_occup << "}," << endl;
					ofs_occup << "{" << endl;
					ofs_occup << "source: 'node-" << i + j * dx +(ii+jj*xx)*dx*dy<< "'," << endl;
					ofs_occup << "target: 'node-" << i + j * dx + dx+(ii+jj*xx)*dx*dy << "'," << endl;
                    current_chanel=channel_connection.find(pair<int,int>(i + j * dx+(ii+jj*xx)*dx*dy,i + j * dx+dx+(ii+jj*xx)*dx*dy));
                    ofs_occup << "id: 'edge-" << current_chanel->second << "'," << endl;
                    ofs_occup << "style:{keyshape: {stroke: 'rgba("<<float(step_occupation[0][current_chanel->second]*255/measure_step)<<", 0, "<<255-float(step_occupation[0][current_chanel->second]*255/measure_step)<<", 1)', },}, " << endl;
					ofs_occup << "}," << endl;
					ofs_occup << "{" << endl;
					ofs_occup << "source: 'node-" << i + j * dx+dx +(ii+jj*xx)*dx*dy<< "'," << endl;
					ofs_occup << "target: 'node-" << i + j * dx+(ii+jj*xx)*dx*dy  << "'," << endl;
                    current_chanel=channel_connection.find(pair<int,int>(i + j * dx+dx+(ii+jj*xx)*dx*dy,i + j * dx+(ii+jj*xx)*dx*dy));
                    ofs_occup << "id: 'edge-" << current_chanel->second << "'," << endl;
                    ofs_occup << "style:{keyshape: {stroke: 'rgba("<<float(step_occupation[0][current_chanel->second]*255/measure_step)<<", 0, "<<255-float(step_occupation[0][current_chanel->second]*255/measure_step)<<", 1)', },}, " << endl;
					ofs_occup << "}," << endl;
					ofs_occup << "{" << endl;
					ofs_occup << "source: 'node-" << i + j * dx +(ii+jj*xx)*dx*dy<< "'," << endl;
					ofs_occup << "target: 'node-" << i + j * dx + dx+(ii+jj*xx)*dx*dy+1 << "'," << endl;
                    current_chanel=channel_connection.find(pair<int,int>(i + j * dx+(ii+jj*xx)*dx*dy,i + j * dx+dx+(ii+jj*xx)*dx*dy+1));
                    ofs_occup << "id: 'edge-" << current_chanel->second << "'," << endl;
                    ofs_occup << "style:{keyshape: {stroke: 'rgba("<<float(step_occupation[0][current_chanel->second]*255/measure_step)<<", 0, "<<255-float(step_occupation[0][current_chanel->second]*255/measure_step)<<", 1)', },}, " << endl;
					ofs_occup << "}," << endl;
					ofs_occup << "{" << endl;
					ofs_occup << "source: 'node-" << i + j * dx+dx +(ii+jj*xx)*dx*dy+1<< "'," << endl;
					ofs_occup << "target: 'node-" << i + j * dx+(ii+jj*xx)*dx*dy  << "'," << endl;
                    current_chanel=channel_connection.find(pair<int,int>(i + j * dx+dx+(ii+jj*xx)*dx*dy+1,i + j * dx+(ii+jj*xx)*dx*dy));
                    ofs_occup << "id: 'edge-" << current_chanel->second << "'," << endl;
                    ofs_occup << "style:{keyshape: {stroke: 'rgba("<<float(step_occupation[0][current_chanel->second]*255/measure_step)<<", 0, "<<255-float(step_occupation[0][current_chanel->second]*255/measure_step)<<", 1)', },}, " << endl;
					ofs_occup << "}," << endl;
				}

			}


			for (int j = 0; j < dy - 1; j++)
			{
					ofs_occup << "{" << endl;
					ofs_occup << "source: 'node-" << dx + j * dx -1+(ii+jj*xx)*dx*dy<< "'," << endl;
					ofs_occup << "target: 'node-" << dx + (j+1) * dx - 1+(ii+jj*xx)*dx*dy << "'," << endl;
                    current_chanel=channel_connection.find(pair<int,int>(dx + j * dx -1+(ii+jj*xx)*dx*dy,dx + (j+1) * dx - 1+(ii+jj*xx)*dx*dy));
                    ofs_occup << "id: 'edge-" << current_chanel->second << "'," << endl;
                    ofs_occup << "style:{keyshape: {stroke: 'rgba("<<float(step_occupation[0][current_chanel->second]*255/measure_step)<<", 0, "<<255-float(step_occupation[0][current_chanel->second]*255/measure_step)<<", 1)', },}, " << endl;
					ofs_occup << "}," << endl;
					ofs_occup << "{" << endl;
					ofs_occup << "source: 'node-" << dx + (j + 1) * dx - 1+(ii+jj*xx)*dx*dy << "'," << endl;
					ofs_occup << "target: 'node-" << dx + j * dx - 1+(ii+jj*xx)*dx*dy << "'," << endl;
                    current_chanel=channel_connection.find(pair<int,int>(dx + (j + 1) * dx - 1+(ii+jj*xx)*dx*dy,dx + j * dx - 1+(ii+jj*xx)*dx*dy));
                    ofs_occup << "id: 'edge-" << current_chanel->second << "'," << endl;
                    ofs_occup << "style:{keyshape: {stroke: 'rgba("<<float(step_occupation[0][current_chanel->second]*255/measure_step)<<", 0, "<<255-float(step_occupation[0][current_chanel->second]*255/measure_step)<<", 1)', },}, " << endl;
					ofs_occup << "}," << endl;

			}

			for (int i = 0; i < dx - 1; i++)
			{
				ofs_occup << "{" << endl;
				ofs_occup << "source: 'node-" << i+(dy-1)*dx+(ii+jj*xx)*dx*dy << "'," << endl;
				ofs_occup << "target: 'node-" << i + (dy - 1)*dx+1+(ii+jj*xx)*dx*dy << "'," << endl;
                current_chanel=channel_connection.find(pair<int,int>(i+(dy-1)*dx+(ii+jj*xx)*dx*dy,i + (dy - 1)*dx+1+(ii+jj*xx)*dx*dy));
                ofs_occup << "id: 'edge-" << current_chanel->second << "'," << endl;
                ofs_occup << "style:{keyshape: {stroke: 'rgba("<<float(step_occupation[0][current_chanel->second]*255/measure_step)<<", 0, "<<255-float(step_occupation[0][current_chanel->second]*255/measure_step)<<", 1)', },}, " << endl;
				ofs_occup << "}," << endl;
				ofs_occup << "{" << endl;
				ofs_occup << "source: 'node-" << i + (dy - 1)*dx+1+(ii+jj*xx)*dx*dy << "'," << endl;
				ofs_occup << "target: 'node-" << i + (dy - 1)*dx+(ii+jj*xx)*dx*dy  << "'," << endl;
                current_chanel=channel_connection.find(pair<int,int>(i + (dy - 1)*dx+1+(ii+jj*xx)*dx*dy,i + (dy - 1)*dx+(ii+jj*xx)*dx*dy));
                ofs_occup << "id: 'edge-" << current_chanel->second << "'," << endl;
                ofs_occup << "style:{keyshape: {stroke: 'rgba("<<float(step_occupation[0][current_chanel->second]*255/measure_step)<<", 0, "<<255-float(step_occupation[0][current_chanel->second]*255/measure_step)<<", 1)', },}, " << endl;
				ofs_occup << "}," << endl;

			}
                            }
            }
			ofs_occup << "]," << endl;
            ofs_occup <<"};"<<endl;
            ofs_occup <<" export default data;"<<endl;
            }
}
