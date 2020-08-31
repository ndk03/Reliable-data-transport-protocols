#include "../include/simulator.h"

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <cstring>
#include <vector>
#include <queue>
#include <deque>
#include <map>
#include <unistd.h>

using namespace std;

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose
 unidirectional data transfer 
   protocols (from A to B). Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

float TIMEOUT = 15.0;

int sequence_number_A; 
int acknowlegde_number_A; 

int window_size_A;
int window_size_B;

int next_sequence_number;
int expected_sequence_B; 
int acknowledge_number_B; 

int send_b;
int recv_b;

struct pkt packet_in_act;

int checksum_generator(pkt packet);
bool corrupt_pkt(pkt packet);
void send_data(int seq_num, bool is_s_r);
void insertionSort (vector<int> data, int n);

struct message_metadata{
	float start;
	bool is_ack;
	char message[20];
};

vector <message_metadata> metadata_buffer;

deque<int> timer_sequence;

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
	float current_time = get_sim_time(); 
	message_metadata msg_meta;
	msg_meta.is_ack = false;
	msg_meta.start = -1;
	strncpy(msg_meta.message,message.data,sizeof(message.data));
	metadata_buffer.push_back(msg_meta);
	
	send_data(-1, false);

}

void send_data(int seq_num, bool is_s_r){
 	
	 if(is_s_r && seq_num >= send_b && seq_num < send_b + window_size_A){	
		pkt packet;
		
		strncpy(packet.payload, metadata_buffer[seq_num].message ,sizeof(packet.payload)); 
		packet.seqnum = seq_num;
		packet.acknum = acknowlegde_number_A;
		packet.checksum = 0;	
		
		packet.checksum = checksum_generator(packet);
		
		tolayer3(0, packet);
		
		metadata_buffer[seq_num].start = get_sim_time();
		
		timer_sequence.push_back(seq_num);
		
		if(timer_sequence.size()==1){ 
			starttimer(0, TIMEOUT);
		} 
	}
	else if(next_sequence_number >= send_b && next_sequence_number < send_b + window_size_A){

		pkt packet; 
		// Reset the current packet for new data
			
		strncpy(packet.payload, metadata_buffer[next_sequence_number].message ,sizeof(packet.payload)); 
		packet.seqnum = next_sequence_number;
		packet.acknum = acknowlegde_number_A;
		packet.checksum = 0;	
		
		packet.checksum = checksum_generator(packet);
		
		tolayer3(0, packet);
		
		metadata_buffer[next_sequence_number].start = get_sim_time();
		timer_sequence.push_back(next_sequence_number);

	
		if(timer_sequence.size()==1){

			starttimer(0, TIMEOUT);
		}		
		
		next_sequence_number++;
	}
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{	
	if(!corrupt_pkt(packet)){
		
		metadata_buffer[packet.acknum].is_ack = true;
		if(send_b == packet.acknum){

			//Move base to the next unacknowledged packet
			while(metadata_buffer.size()>send_b && metadata_buffer[send_b].is_ack)
				send_b++;
			
			while(next_sequence_number < send_b + window_size_A && next_sequence_number < metadata_buffer.size()){
				send_data(-1, false);
			}	
		}
		
		if(timer_sequence.front() == packet.acknum){
			
			stoptimer(0);
			timer_sequence.pop_front();
			
			while(timer_sequence.size() > 0 && timer_sequence.size() <= window_size_A && metadata_buffer[timer_sequence.front()].is_ack){
			
				timer_sequence.pop_front();
			}
			
			if(timer_sequence.size() > 0 && timer_sequence.size() <= window_size_A){				
				float next_timeout = TIMEOUT - (get_sim_time() - metadata_buffer[timer_sequence.front()].start);				
				starttimer(0, next_timeout);
			}
		}
	}
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
	
	int interrupted_sequence = timer_sequence.front();
	timer_sequence.pop_front();
	while(timer_sequence.size() > 0 && timer_sequence.size() <= window_size_A && metadata_buffer[timer_sequence.front()].is_ack){
		
		timer_sequence.pop_front();
	}
	
	if(timer_sequence.size() > 0 && timer_sequence.size() <= window_size_A){
		float next_timeout = TIMEOUT - (get_sim_time() - metadata_buffer[timer_sequence.front()].start);
		starttimer(0, next_timeout);
	}
	send_data(interrupted_sequence, true);
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
	sequence_number_A = 0; 
	acknowlegde_number_A = 0; 
	window_size_A = getwinsize()/2;
	send_b = 0;
	next_sequence_number = 0;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */


map<int, pkt> received_buffer;

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
	if(!corrupt_pkt(packet) && recv_b == packet.seqnum){
		tolayer5(1, packet.payload);
		pkt received_acknowledgement;
		received_acknowledgement.acknum = packet.seqnum;
		received_acknowledgement.checksum = 0;
		received_acknowledgement.checksum = checksum_generator(received_acknowledgement);
		tolayer3(1, received_acknowledgement);	
		recv_b++;
		}

	else if(!corrupt_pkt(packet) &&  packet.seqnum >= recv_b && packet.seqnum < recv_b+window_size_B){
		received_buffer[packet.seqnum] = packet;
		pkt received_acknowledgement;
		received_acknowledgement.acknum = packet.seqnum;
		received_acknowledgement.checksum = 0;
		received_acknowledgement.checksum = checksum_generator(received_acknowledgement);
		tolayer3(1, received_acknowledgement);	
	}
	else if(!corrupt_pkt(packet)){
		pkt received_acknowledgement;
		received_acknowledgement.acknum = packet.seqnum;
		received_acknowledgement.checksum = 0;
		received_acknowledgement.checksum = checksum_generator(received_acknowledgement);
		tolayer3(1, received_acknowledgement);	
	}
	
	 map<int, pkt>::iterator it;
	 it = received_buffer.find(recv_b);
	
	while (it != received_buffer.end()){
		pkt buffered_pkt = it->second;
		char buffered_data[20];
		strncpy(buffered_data, buffered_pkt.payload ,sizeof(buffered_data));
		 tolayer5(1, buffered_data);
		received_buffer.erase(it);
		recv_b++;
		it = received_buffer.find(recv_b);
	}
		
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
	expected_sequence_B = 0;
	window_size_B = getwinsize()/2;
	recv_b = 0;
}

int checksum_generator(pkt packet){
	int checksum = 0;
	int seq_number = packet.seqnum;
	int ack_number = packet.acknum;
	checksum += seq_number;
	checksum += ack_number;
	
	for(int i=0; i<sizeof(packet.payload); i++)
		checksum += packet.payload[i];
	
	return ~checksum;
}

bool corrupt_pkt(pkt packet){
	
	if(packet.checksum == checksum_generator(packet))
		return false;
	else
		return true;
		
}
