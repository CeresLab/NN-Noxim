/*
 * Noxim - the NoC Simulator
 *
 * (C) 2005-2010 by the University of Catania
 * For the complete list of authors refer to file ../doc/AUTHORS.txt
 * For the license applied to these sources refer to file ../doc/LICENSE.txt
 *
 * This file contains the implementation of the processing element
 */
/*
 * NN-Noxim - the NoC-based ANN Simulator
 *
 * (C) 2018 by National Sun Yat-sen University in Taiwan
 *
 * This file contains the implementation of loading NN model
 */

#include <iomanip>
#include "fixedp.h"
#include "NoximProcessingElement.h"
//extern int throttling[8][8][4];
//extern int throttling[DEFAULT_MESH_DIM_X][DEFAULT_MESH_DIM_Y][DEFAULT_MESH_DIM_Z];

int NoximProcessingElement::randInt(int min, int max)
{
	return min +
		(int) ((double) (max - min + 1) * rand() / (RAND_MAX + 1.0));
}

void NoximProcessingElement::rxProcess()
{	
	if (reset.read() ) {
		//ack_rx.write(0);
		//current_level_rx = 0;
		ack_rx.write(1);
//***************NN-Noxim********************************reset_1

   		if(reset.read())
		{
			char fileID_r[10];
			sprintf(fileID_r,"%d",local_id);
			char file_name_r[10] = "PE_R_";
			strcat(file_name_r, fileID_r);
			remove(file_name_r);
		}

//**********************^^^^^^^^^^^^^^**************************
    	} 
	else {
		//if (req_rx.read() == 1 - current_level_rx) {
		if(req_rx.read()==1){

			NoximFlit flit_tmp = flit_rx.read();
			
	    		//if (NoximGlobalParams::verbose_mode > VERBOSE_OFF) {
	    		if (1) {
				char fileID_r[10];
				sprintf(fileID_r,"%d",local_id);
				char file_name_r[10] = "PE_R_";
				strcat(file_name_r, fileID_r);
				fstream file_r;
				file_r.open( file_name_r ,ios::out|ios::app);
				file_r << sc_simulation_time() << ": ProcessingElement[" <<local_id << "] RECEIVING " << flit_tmp << endl;
				//cout << sc_simulation_time() << ": ProcessingElement[" <<local_id << "] RECEIVING " << flit_tmp << endl;
			}
			// current_level_rx = 1 - current_level_rx;		// Negate the old value for Alternating Bit Protocol (ABP)
			
//***************NN-Noxim*************************receive & compute
			if (flit_tmp.flit_type == FLIT_TYPE_BODY)
			{
				int point_receive_Neu_ID = -1;
				for (int i = 0 ; i<receive ; i++)
				{
					if (receive_Neu_ID[i] == flit_tmp.src_Neu_id)
					{
						point_receive_Neu_ID = i;
						should_receive--;
						break;
					}
				}
				receive_data[point_receive_Neu_ID]=flit_tmp.data;

				if (should_receive == 0)
				{
					cout<<sc_simulation_time()<<": (PE_"<<local_id<<") Now layer "<<ID_layer<<" start computing..."<<endl;
					//*****************************computing*******************************		

					int start_ID_last_layer = receive_Neu_ID[0];
					int x_size_last_layer = NN_Model->all_leyer_size[ID_layer-1][1];
					int y_size_last_layer = NN_Model->all_leyer_size[ID_layer-1][2];
					int x_size_layer = NN_Model->all_leyer_size[ID_layer][1];
					int n_size_layer = NN_Model->all_leyer_size[ID_layer][3];
					for (int j = 0 ; j<receive ; j++)
					{					
						for (int i = 0 ; i<Use_Neu ; i++)
						{
							//********************fully connected********************** //
							if(Type_layer =='f')
							{
								float weight_tmp = PE_table[i].weight[j];
								res[i] += receive_data[j] * weight_tmp;

								if (j==receive-1) 
								{
									float bias_tmp = PE_table[i].weight.back();
									res[i] += bias_tmp;					//act fun & compute complete 

									if ( NN_Model->all_leyer_size[ID_layer].back() == RELU )//relu
									{
										if (res[i] <= 0) 
											res[i]=0;				
									}
									else if ( NN_Model->all_leyer_size[ID_layer].back() == TANH )//tanh
									{
										res[i]= 2/(1+exp(-2*res[i]))-1;		
									}
									else if ( NN_Model->all_leyer_size[ID_layer].back() == SIGMOID )//sigmoid
									{
										res[i]= 1/(1+exp(-1*res[i]));	
									}
									else if ( NN_Model->all_leyer_size[ID_layer].back() == SOFTMAX )//softmax
									{}

									if (ID_layer == NN_Model->all_leyer_size.size()-1)
									{
										//cout << sc_simulation_time() + computation_time <<": The prediction result of item "<< PE_table[i].ID_In_layer << " : " << res[i] << endl;
										char output_file[10];
										sprintf(output_file,"output.txt");
										fstream file_o;
										file_o.open( output_file ,ios::out|ios::app);
										file_o << "No." << i << " output neuron result: ";
										file_o << res[i] << endl;
									}
								}
							}
						}
					}
					flag_p = 1; 
					flag_f = 1; 
					temp_computation_time = sc_simulation_time();
				}
			}
//**********************^^^^^^^^^^^^^^**************************
		}
		//ack_rx.write(current_level_rx);
		ack_rx.write(1);
    	}
}

void NoximProcessingElement::txProcess()
{
if (reset.read() ) {
		req_tx.write(0);
		current_level_tx = 0;
		transmittedAtPreviousCycle = false;
		not_transmit=0;
		transmit=0;
		adaptive_transmit=0;
		dor_transmit=0;
		dw_transmit =0;
//********************NN-Noxim*****************tytyty****************reset_2
		
		PE_enable = 0;	
		ID_layer = -1;
		ID_group = 0;	//** 2018.09.17 edit by Yueh-Chi,Yang **//

		res.clear();

		receive = 0;
		receive_Neu_ID.clear();
		
		Use_Neu	= 0;
		Use_Neu_ID.clear();
		trans = 0;
		trans_PE_ID.clear();

		my_data_in.clear();
		PE_Weight.clear();
   
   		if(!reset.read())
			in_data++;
		else
		{
			char fileID_t[10];
			sprintf(fileID_t,"%d",local_id);
			char file_name_t[10] = "PE_T_";
			strcat(file_name_t, fileID_t);
			remove(file_name_t);
			char output_file[10];
			sprintf(output_file,"output.txt");
			remove(output_file);
			in_data = 0;
		}


		for( int k = 0 ; k<NN_Model->mapping_table.size() ; k++ )
		{
			if(NN_Model->mapping_table[k]==local_id)
			{
				ID_group = k;
				if(ID_group<NN_Model->Group_table.size())
				{
					PE_enable = 1;

					PE_table = NN_Model->Group_table[ID_group];
					//deque<NeuInformation>().swap(NN_Model->Group_table[ID_group]);
					
					ID_layer = PE_table[0].ID_layer;
					Type_layer = PE_table[0].Type_layer;

					Use_Neu = PE_table.size();
					for(int i = 0 ; i<Use_Neu ; i++)
					{
						Use_Neu_ID.push_back(PE_table[i].ID_Neu);
					}

					if(Type_layer != 'i')		//set should receive
					{
						flag_p = 0;
						flag_f = 0;
						receive = NN_Model-> all_leyer_size[ID_layer-1][0];
						should_receive = receive;
						receive_Neu_ID.clear();
						receive_data.assign(receive , 0 );
						
						int temp_receive_start_ID = Use_Neu_ID[0] - PE_table[0].ID_In_layer - receive;
						
						for(int i = 0 ; i<receive ; i++)
						{
							receive_Neu_ID.push_back(temp_receive_start_ID+i);
						}
					}
					else
					{
						flag_p=1;
						flag_f=1;
					}
					
					if (ID_layer != NN_Model->all_leyer_size.size()-1)		//set should sent
					{
						int i;
						for(i = 0 ; i<NN_Model->all_leyer_ID_Group[ID_layer+1].size() ; i++)
						{
							int temp_Group = NN_Model->all_leyer_ID_Group[ID_layer+1][i];
							trans_PE_ID.push_back(NN_Model-> mapping_table[temp_Group]);
						}
						trans = i;
						should_trans = trans;
					}

					if (Type_layer == 'i')		//set input
					{
					      	deque< float> my_temp_data_in;
					      	for(int j = 0; j<NN_Model->all_data_in.size() ; j++)
							{
								for(int i = 0; i<Use_Neu ; i++)
								{
									my_temp_data_in.push_back( NN_Model->all_data_in[j][ Use_Neu_ID[i] ] );

								}	
								my_data_in.push_back(my_temp_data_in);
								my_temp_data_in.clear();
					      	}
					}
					else
					{
						res.assign( Use_Neu, 0 );
					}
				}
				break;
			}
		}
		computation_time = NoximGlobalParams::PE_computation_time;

//**************************^^^^^^^^^^^^^^^^^^^^^**************************
    	}
	else {
		NoximPacket packet;
//********************NN-Noxim*****************tytyty****************trans
		if(clean_all == false){
			if(!throttle_local){

				if(PE_enable && ID_layer != NN_Model->all_leyer_size.size()-1 && flag_p ){
					//**** 2018.09.17 edit by Yueh-Chi,Yang ****//
					cout<<sc_simulation_time()<<": (PE_"<<local_id<<") Now layer "<<ID_layer<<" start sending..."<<endl;

					for(int i = 0; i<trans ; i++){
						packet.make(local_id, trans_PE_ID[i], getCurrentCycleNum(), Use_Neu+2);
						packet_queue.push(packet);
					}
					flag_p = 0;
					transmittedAtPreviousCycle = true;
				} 
				else
					transmittedAtPreviousCycle = false;
			}
		}

		if(ack_tx.read() == 1  && flag_f && PE_enable && ID_layer != NN_Model->all_leyer_size.size()-1 && (sc_simulation_time() >= temp_computation_time + computation_time||ID_layer==0) ){
			if (!packet_queue.empty()) {
				//NoximFlit flit = nextFlit();	// Generate a new flit
				
				NoximFlit flit = nextFlit(ID_layer, in_data);	// tytyty Generate a new flit
        			//NoximFlit flit = nextFlit(ID_layer);	// tytyty Generate a new flit
				//if (NoximGlobalParams::verbose_mode > VERBOSE_OFF) {
				if (1) {
					char fileID_t[10];
					sprintf(fileID_t,"%d",local_id);
					char file_name_t[10] = "PE_T_";
					strcat(file_name_t, fileID_t);
    					fstream file_t;
					file_t.open( file_name_t ,ios::out|ios::app);
					file_t<< getCurrentCycleNum() << ": ProcessingElement[" << local_id <<"] SENDING " << flit << endl;
				}
				
				flit_tx->write(flit);	// Send the generated flit
				//current_level_tx = 1 - current_level_tx;	// Negate the old value for Alternating Bit Protocol (ABP)
				//req_tx.write(current_level_tx);
				req_tx.write(1);
				if (flit.flit_type==FLIT_TYPE_HEAD) 
					cnt_local++;
			}
			else{
				req_tx.write(0);
				flag_f = 0;
			}
		}	
//**************************^^^^^^^^^^^^^^^^^^^^^**************************
}
}

//NoximFlit NoximProcessingElement::nextFlit()
NoximFlit NoximProcessingElement::nextFlit(const int ID_layer, const int in_data)  //************tyty*****
//NoximFlit NoximProcessingElement::nextFlit(const int ID_layer)  //************tyty*****
{
    NoximFlit flit;
    NoximPacket packet = packet_queue.front();

    flit.src_id = packet.src_id;
    flit.dst_id = packet.dst_id;
    flit.timestamp = packet.timestamp;
    flit.sequence_no = packet.size - packet.flit_left;
    flit.hop_no = 0;
	flit.routing_f   = packet.routing;
//**********************tytyty***************************
	if(ID_layer%2)
		flit.XYX_routing   = 0;	//YX_routing
	else
		flit.XYX_routing   = 1; //XY_routing
//*******************************************************
    //  flit.payload     = DEFAULT_PAYLOAD;

    if (packet.size == packet.flit_left) {
		flit.flit_type = FLIT_TYPE_HEAD;
		flit.src_Neu_id = 0;
		flit.data = 0;
	}
	
    else if (packet.flit_left == 1) {
		flit.flit_type = FLIT_TYPE_TAIL;
		flit.src_Neu_id = 0;
		flit.data = 0;
	}
	
//************************NN-Noxim*****************tytyty**************setflit_data
    else{
	flit.flit_type = FLIT_TYPE_BODY;
	flit.src_Neu_id = Use_Neu_ID[flit.sequence_no-1];
	if(Type_layer=='i')
		flit.data = my_data_in[in_data][flit.sequence_no-1];
 		//flit.data = my_data_in[flit.sequence_no-1];
	else
		flit.data = res[flit.sequence_no-1];
    }
//************************************^^^^^^^^^^^^^^^^^^^^^^^^*********************

    packet_queue.front().flit_left--;
    if (packet_queue.front().flit_left == 0)
	packet_queue.pop();

    return flit;
}

bool NoximProcessingElement::canShot(NoximPacket & packet)
{
    bool shot;
    double threshold;

    if (NoximGlobalParams::traffic_distribution != TRAFFIC_TABLE_BASED) 
	{
	if (!transmittedAtPreviousCycle)
		threshold = NoximGlobalParams::packet_injection_rate;
	else
	    threshold = NoximGlobalParams::probability_of_retransmission;
	
	shot = (((double) rand()) / RAND_MAX < threshold);
	if (shot) {
	    switch (NoximGlobalParams::traffic_distribution) {
	    case TRAFFIC_RANDOM:
		packet = trafficRandom();
		break;

	    case TRAFFIC_TRANSPOSE1:
		packet = trafficTranspose1();
		break;

	    case TRAFFIC_TRANSPOSE2:
		packet = trafficTranspose2();
		break;

	    case TRAFFIC_BIT_REVERSAL:
		packet = trafficBitReversal();
		break;

	    case TRAFFIC_SHUFFLE:
		packet = trafficShuffle();
		break;

	    case TRAFFIC_BUTTERFLY:
		packet = trafficButterfly();
		break;

	    default:
		assert(false);
	    }
	}
    }else 
	{			// Table based communication traffic
		if (never_transmit)
			return false;

		int now = getCurrentCycleNum() ;
		bool use_pir = (transmittedAtPreviousCycle == false);
		vector < pair < int, double > > dst_prob;
		double threshold =
			traffic_table->getCumulativePirPor(local_id, now,
							   use_pir, dst_prob);

		double prob = (double) rand() / RAND_MAX;
		shot = (prob < threshold);
		if (shot) 
		{
			for (unsigned int i = 0; i < dst_prob.size(); i++) {
			if (prob < dst_prob[i].second) {
				packet.make(local_id, dst_prob[i].first, now,
					getRandomSize());
				break;
			}
			}
		}
    }

    return shot;
}


NoximPacket NoximProcessingElement::trafficRandom()
{
    int max_id = (NoximGlobalParams::mesh_dim_x * NoximGlobalParams::mesh_dim_y*NoximGlobalParams::mesh_dim_z) - 1;
	NoximPacket p;
    p.src_id = local_id;
    double rnd = rand() / (double) RAND_MAX;
    double range_start = 0.0;

    //cout << "\n " << getCurrentCycleNum() << " PE " << local_id << " rnd = " << rnd << endl;

    
	 int re_transmit=1; //1

    // Random destination distribution
    do {
	transmit++;
	p.dst_id = randInt(0, max_id);

	// check for hotspot destination
	for (unsigned int i = 0; i < NoximGlobalParams::hotspots.size(); i++) {
	    //cout << getCurrentCycleNum() << " PE " << local_id << " Checking node " << NoximGlobalParams::hotspots[i].first << " with P = " << NoximGlobalParams::hotspots[i].second << endl;

	    if (rnd >= range_start
		&& rnd <
		range_start + NoximGlobalParams::hotspots[i].second) {
		if (local_id != NoximGlobalParams::hotspots[i].first) {
		    //cout << getCurrentCycleNum() << " PE " << local_id <<" That is ! " << endl;
		    p.dst_id = NoximGlobalParams::hotspots[i].first;
		}
		break;
	    } else
		range_start += NoximGlobalParams::hotspots[i].second;	// try next
	}
	
	/////////////////////////////Matthew: Cross Layer Solution////////////////////////////
 
 re_transmit=1;  //1

 NoximCoord current = id2Coord(local_id);
 NoximCoord destination = id2Coord(p.dst_id);

 int x_diff=destination.x-current.x;
 int y_diff=destination.y-current.y;
 int z_diff=destination.z-current.z;

 int n_x;
 int n_y;
 int n_z;
 int a_x_search;
 int a_y_search;
 int a_z_search;
 
 int in_xy = 0;  //0
 int c=0;
 int xy_fail=0;
 int adaptive_not_ok=0;  //0
 int routing =ROUTING_XYZ;
 int destination_throttle=1;
 int dw =0;

 if(NoximGlobalParams::throttling[destination.x][destination.y][destination.z] == 0)
 {
   destination_throttle =0;
    
    if(x_diff>=0 && y_diff>=0)
		 {
		    int x_a_diff = x_diff;
			int y_a_diff = y_diff;
		    for(int y_a=0;y_a<y_a_diff+1;y_a++)
			{
			  for(int x_a=0;x_a<x_a_diff+1;x_a++)
			  {
			    a_x_search=current.x+x_a;
				a_y_search=current.y+y_a;
				if(NoximGlobalParams::throttling[a_x_search][a_y_search][current.z]==1)
				   adaptive_not_ok++;
			  }
			}
		 }
		 else if(x_diff>=0 && y_diff<0)
		 {
		    int x_a_diff = x_diff;
			int y_a_diff = -y_diff;
		    for(int y_a=0;y_a<y_a_diff+1;y_a++)
			{
			  for(int x_a=0;x_a<x_a_diff+1;x_a++)
			  {
			    a_x_search=current.x+x_a;
				a_y_search=current.y-y_a;
				if(NoximGlobalParams::throttling[a_x_search][a_y_search][current.z]==1)
				   adaptive_not_ok++;
			  }
			}
		 }
		 else if(x_diff<0 && y_diff>=0)
		 {
		    int x_a_diff = -x_diff;
			int y_a_diff = y_diff;
		    for(int y_a=0;y_a<y_a_diff+1;y_a++)
			{
			  for(int x_a=0;x_a<x_a_diff+1;x_a++)
			  {
			    a_x_search=current.x-x_a;
				a_y_search=current.y+y_a;
				if(NoximGlobalParams::throttling[a_x_search][a_y_search][current.z]==1)
				   adaptive_not_ok++;
			  }
			}
		 }
		 else //(x_diff<0 && y_diff<0)
		 {
		    int x_a_diff = -x_diff;
			int y_a_diff = -y_diff;
		    for(int y_a=0;y_a<y_a_diff+1;y_a++)
			{
			  for(int x_a=0;x_a<x_a_diff+1;x_a++)
			  {
			    a_x_search=current.x-x_a;
				a_y_search=current.y-y_a;
				if(NoximGlobalParams::throttling[a_x_search][a_y_search][current.z]==1)
				   adaptive_not_ok++;
			  }
			}
		 }

		 if(z_diff>=0)
		 {
            for(int zt=1;zt<z_diff+1;zt++)
		    {
				a_z_search=current.z+zt;
				if(NoximGlobalParams::throttling[destination.x][destination.y][a_z_search]==1)
					adaptive_not_ok++;
		    }
		 }
		 else
		 {
			 int z_diff_tt = -z_diff;
			 for(int zt=1;zt<z_diff_tt+1;zt++)
		    {
				a_z_search=current.z-zt;
				if(NoximGlobalParams::throttling[destination.x][destination.y][a_z_search]==1)
					adaptive_not_ok++;
		    }
		 }


		 
		 
		 if(adaptive_not_ok >= 1)
		   in_xy=1;
		 else
		   in_xy=0;

	 ////////////////////////�i�JXY Routing//////////////
	if(in_xy==1)
	{
		 if(x_diff>=0)
		 {
            for(int xt=1;xt<x_diff+1;xt++)
		    {
				n_x=current.x+xt;
				if(NoximGlobalParams::throttling[n_x][current.y][current.z]==1)
					c++;
		    }
		 }
		 else
		 {
			 int x_diff_t=-x_diff;
			 for(int xt=1;xt<x_diff_t+1;xt++)
		    {
				n_x=current.x-xt;
				if(NoximGlobalParams::throttling[n_x][current.y][current.z]==1)
					c++;
		    }
		 }

		 if(y_diff>=0)
		 {
            for(int yt=1;yt<y_diff+1;yt++)
		    {
				n_y=current.y+yt;
				if(NoximGlobalParams::throttling[destination.x][n_y][current.z]==1)
					c++;
		    }
		 }
		 else
		 {
			 int y_diff_t = -y_diff;
			 for(int yt=1;yt<y_diff_t+1;yt++)
		    {
				n_y=current.y-yt;
				if(NoximGlobalParams::throttling[destination.x][n_y][current.z]==1)
					c++;
		    }
		 }

		 if(z_diff>=0)
		 {
            for(int zt=1;zt<z_diff+1;zt++)
		    {
				n_z=current.z+zt;
				if(NoximGlobalParams::throttling[destination.x][destination.y][n_z]==1)
					c++;
		    }
		 }
		 else
		 {
			 int z_diff_t = -z_diff;
			 for(int zt=1;zt<z_diff_t+1;zt++)
		    {
				n_z=current.z-zt;
				if(NoximGlobalParams::throttling[destination.x][destination.y][n_z]==1)
					c++;
		    }
		 }

		 if(c>=1)
			 xy_fail=1;
		 else
			 xy_fail=0;


	 }


     ////////////////////////�i�JDownward Routing//////////////
	 
	 if(xy_fail>=1)
	 {
	    int z_diff_dw_s = (NoximGlobalParams::mesh_dim_z-1) - current.z;
	      for(int zzt=1;zzt<z_diff_dw_s+1;zzt++)
		    {
				n_z=current.z+zzt;
				if(NoximGlobalParams::throttling[current.x][current.y][n_z]==1)
					dw++;
		    }
			
		int z_diff_dw_d = (NoximGlobalParams::mesh_dim_z-1) - destination.z;
	      for(int zzt=1;zzt<z_diff_dw_d+1;zzt++)
		    {
				n_z=destination.z+zzt;
				if(NoximGlobalParams::throttling[destination.x][destination.y][n_z]==1)
					dw++;
		    }
	 
	 }
	 
	 
	 if(adaptive_not_ok==0)
		{
		   re_transmit=0;
		   routing =ROUTING_WEST_FIRST; //ROUTING_WEST_FIRST
		   adaptive_transmit++;
		}
		else if(adaptive_not_ok >=1 && xy_fail==0)
		{
		   re_transmit=0;//0
		   routing =ROUTING_XYZ;
		   dor_transmit++;
		}
		else if(adaptive_not_ok >=1 && xy_fail >=1 && dw==0)
		{
		   re_transmit=0;
		   routing =ROUTING_DOWNWARD_CROSS_LAYER;
		   dw_transmit++;
		}
		else if(adaptive_not_ok >=1 && xy_fail >=1 && dw>=1)
		{
		   re_transmit=1;
		   routing =ROUTING_DOWNWARD_CROSS_LAYER;
		}
		 
		p.routing=routing;
		
     }//if(throttling[destination.x][destination.y] == 0)


       if(re_transmit==1)
		 not_transmit++;
	
	
    } while ((p.dst_id == p.src_id) || (re_transmit));

    p.timestamp = getCurrentCycleNum() ;
    p.size = p.flit_left = getRandomSize();

    return p;
}



NoximPacket NoximProcessingElement::trafficTranspose1()
{
    NoximPacket p;
    p.src_id = local_id;
    NoximCoord src, dst;

    // Transpose 1 destination distribution
    src.x = id2Coord(p.src_id).x;
    src.y = id2Coord(p.src_id).y;
    src.z = id2Coord(p.src_id).z;
	dst.x = NoximGlobalParams::mesh_dim_x - 1 - src.y;
    dst.y = NoximGlobalParams::mesh_dim_y - 1 - src.x;
    dst.z = NoximGlobalParams::mesh_dim_z-1-src.z;
	fixRanges(src, dst);
    p.dst_id = coord2Id(dst);

    p.timestamp = getCurrentCycleNum() ;
    p.size = p.flit_left = getRandomSize();

    return p;
}

NoximPacket NoximProcessingElement::trafficTranspose2()
{
    NoximPacket p;
    p.src_id = local_id;
    NoximCoord src, dst;

    // Transpose 2 destination distribution
    src.x = id2Coord(p.src_id).x;
    src.y = id2Coord(p.src_id).y;
    dst.x = src.y;
    dst.y = src.x;
    fixRanges(src, dst);
    p.dst_id = coord2Id(dst);

    p.timestamp = getCurrentCycleNum();
    p.size = p.flit_left = getRandomSize();

    return p;
}

void NoximProcessingElement::setBit(int &x, int w, int v)
{
    int mask = 1 << w;

    if (v == 1)
	x = x | mask;
    else if (v == 0)
	x = x & ~mask;
    else
	assert(false);
}

int NoximProcessingElement::getBit(int x, int w)
{
    return (x >> w) & 1;
}

inline double NoximProcessingElement::log2ceil(double x)
{
    return ceil(log(x) / log(2.0));
}

NoximPacket NoximProcessingElement::trafficBitReversal()
{

    int nbits =
	(int)
	log2ceil((double)
		 (NoximGlobalParams::mesh_dim_x *
		  NoximGlobalParams::mesh_dim_y));
    int dnode = 0;
    for (int i = 0; i < nbits; i++)
	setBit(dnode, i, getBit(local_id, nbits - i - 1));

    NoximPacket p;
    p.src_id = local_id;
    p.dst_id = dnode;

    p.timestamp = getCurrentCycleNum();
    p.size = p.flit_left = getRandomSize();

    return p;
}

NoximPacket NoximProcessingElement::trafficShuffle()
{

    int nbits =
	(int)
	log2ceil((double)
		 (NoximGlobalParams::mesh_dim_x *
		  NoximGlobalParams::mesh_dim_y));
    int dnode = 0;
    for (int i = 0; i < nbits - 1; i++)
	setBit(dnode, i + 1, getBit(local_id, i));
    setBit(dnode, 0, getBit(local_id, nbits - 1));

    NoximPacket p;
    p.src_id = local_id;
    p.dst_id = dnode;

    p.timestamp = getCurrentCycleNum();
    p.size = p.flit_left = getRandomSize();

    return p;
}

NoximPacket NoximProcessingElement::trafficButterfly()
{

    int nbits =
	(int)
	log2ceil((double)
		 (NoximGlobalParams::mesh_dim_x *
		  NoximGlobalParams::mesh_dim_y));
    int dnode = 0;
    for (int i = 1; i < nbits - 1; i++)
	setBit(dnode, i, getBit(local_id, i));
    setBit(dnode, 0, getBit(local_id, nbits - 1));
    setBit(dnode, nbits - 1, getBit(local_id, 0));

    NoximPacket p;
    p.src_id = local_id;
    p.dst_id = dnode;

    p.timestamp = getCurrentCycleNum() ;
    p.size = p.flit_left = getRandomSize();

    return p;
}

void NoximProcessingElement::fixRanges(const NoximCoord src,
				       NoximCoord & dst)
{
    // Fix ranges
    if (dst.x < 0)
	dst.x = 0;
    if (dst.y < 0)
	dst.y = 0;
    if (dst.x >= NoximGlobalParams::mesh_dim_x)
	dst.x = NoximGlobalParams::mesh_dim_x - 1;
    if (dst.y >= NoximGlobalParams::mesh_dim_y)
	dst.y = NoximGlobalParams::mesh_dim_y - 1;
	if (dst.z >= NoximGlobalParams::mesh_dim_z)
	dst.z = NoximGlobalParams::mesh_dim_z - 1;
}

int NoximProcessingElement::getRandomSize()
{
    return randInt(NoximGlobalParams::min_packet_size,
		   NoximGlobalParams::max_packet_size);
}
/***MODIFY BY HUI-SHUN***/
NoximPacket NoximProcessingElement::trafficRandom_Tvar()
{
  int max_id = (NoximGlobalParams::mesh_dim_x * NoximGlobalParams::mesh_dim_y * NoximGlobalParams::mesh_dim_z)-1;////
  NoximPacket p;
  p.src_id = local_id;//randInt(0, max_id);//
  double rnd = rand()/(double)RAND_MAX;
  double range_start = 0.0;

  //cout << "\n " << getCurrentCycleNum() << " PE " << local_id << " rnd = " << rnd << endl;

   

  // Random destination distribution
  do
  {
		p.dst_id = randInt(0, max_id);

    // check for hotspot destination
    for (unsigned int i = 0; i<NoximGlobalParams::hotspots.size(); i++)
    {
	//cout << getCurrentCycleNum() << " PE " << local_id << " Checking node " << TGlobalParams::hotspots[i].first << " with P = " << TGlobalParams::hotspots[i].second << endl;

	if (rnd>=range_start && rnd < range_start + NoximGlobalParams::hotspots[i].second)
	{
	    if (local_id != NoximGlobalParams::hotspots[i].first)
	    {
		//cout << getCurrentCycleNum() << " PE " << local_id <<" That is ! " << endl;
		p.dst_id = NoximGlobalParams::hotspots[i].first;
	    }
	    break;
	}
	else 
	    range_start+=NoximGlobalParams::hotspots[i].second; // try next
    }
  } while(p.dst_id==p.src_id);

  p.timestamp = getCurrentCycleNum();
  p.size = p.flit_left = getRandomSize();

  return p;
}


void NoximProcessingElement::TraffThrottlingProcess()
{
	if(NoximGlobalParams::throt_type == THROT_NORMAL)
		throttle_local = false;
	else if(NoximGlobalParams::throt_type == THROT_TEST)
	{	
		if(!emergency)
			throttle_local = false;
		else //emergency mode
		{		
				throttle_local=true;
		}		
	}
	else if(NoximGlobalParams::throt_type == THROT_VERTICAL)
	{	
		if(!emergency)
			throttle_local = false;
		else //emergency mode
		{		
			if(cnt_local >= Quota_local*Q_ratio )
			{
				throttle_local=true;
//				cout<<getCurrentCycleNum()<<": Local port of Router "<<local_id<<" are throttled!"<<endl;
			}
			else
				throttle_local=false;
		}		
	}
	else
	{	
		if(!emergency)
			throttle_local = false;
		else //emergency mode
		{		
			if(cnt_local >= Quota_local )
			{
				throttle_local=true;
//				cout<<getCurrentCycleNum()<<": Local port of Router "<<local_id<<" are throttled!"<<endl;
			}
			else
				throttle_local=false;
		}		
	}
}
