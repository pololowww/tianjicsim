#include "anynet_file_create.hpp"
#include <fstream>

using namespace std;
int readanynet( const Configuration & config ) {
	int x=config.GetInt("core_x");
    int y=config.GetInt("core_y");
    int xx=config.GetInt("chip_x");
    int yy=config.GetInt("chip_y");
	string topo=config.GetStr( "topo" );
	ofstream out("./examples/anynet/anynet_file");
	int dx=x;
	int dy=y;
	if (out.is_open())
	{


		for (int x = 0; x < xx*yy; x++) {
			for (int j = 0; j < dy - 1; j++)
			{
				for (int i = 0; i < dx - 1; i++) {
					if(topo=="mesh"||topo=="torus")
					{out << "router " <<dx*dy*x+ i + dx * j << " node " << dx * dy*x + i + dx * j << " router " << dx * dy*x + i + 1 + dx * j << " router " << dx * dy*x + i + dx + dx * j << "\n";}
					else{
						out << "router " <<dx*dy*x+ i + dx * j << " node " << dx * dy*x + i + dx * j << " router " << dx * dy*x + i + 1 + dx * j << " router " << dx * dy*x + i + dx + dx * j << " router " << dx * dy*x + i + dx + dx * j + 1<<"\n";
					}
				}
				out << "router " << dx * dy*x + dx - 1 + dx * j << " node " << dx * dy*x + dx - 1 + dx * j << " router " << dx * dy*x + 2 * dx - 1 + dx * j ;
				if(topo=="torus")out<<" router " << dx * dy*x + dx * j;
				out<<  "\n";
			}
			for (int i = 0; i < dx - 1; i++) {
				out << "router " << dx * dy*x + i + dx * (dy - 1) << " node " << dx * dy*x + i + dx * (dy - 1) << " router " << dx * dy*x + i + 1 + dx * (dy - 1) <<  "\n";
			}
			out << "router " << dx * dy*x + dx * dy - 1 << " node " << dx * dy*x + dx * dy - 1 ;
			if(topo=="torus")out<<" router " << dx * dy*x + dx * (dy-1);
			out<< "\n";
		}
		for (int x = 0; x < xx*yy; x++) {
			out << "router " << dx * dy*xx*yy + x;
			for (int j = 0; j < dx; j++)
			{
				out <<" router " << dx * dy*x + j << " router " << dx * dy*x + j + dx * (dy - 1);
			}
			for (int i = 1; i < dy - 1; i++)
			{
				out << " router " << dx * dy*x + i * dx << " router " << dx * dy*x + i * dx + dx - 1;
			}
			out << "\n";
		}
		for (int j = 0; j < yy - 1; j++)
		{
			for (int i = 0; i < xx - 1; i++) {
				out << "router " << dx * dy*xx*yy + i + xx * j << " router " << dx * dy*xx*yy + i + 1 + xx * j << " router " << dx * dy*xx*yy + i + xx + xx * j << "\n";
			}
			out << "router " << dx * dy*xx*yy + xx - 1 + xx * j << " router " << dx * dy*xx*yy + 2 * xx - 1 + xx * j << "\n";
		}
		for (int i = 0; i < xx - 1; i++) {
			out << "router " << dx * dy*xx*yy + i + xx * (yy - 1) << " router " << dx * dy*xx*yy + i + 1 + xx * (yy - 1) << "\n";
		}
		out.close();
	}
	return ((dx*dy+1)*xx*yy);
}
