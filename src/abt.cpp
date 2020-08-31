#include "../include/simulator.h"

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <cstring>
#include <queue>

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

/* called from layer 5, passed the data to be sent to other side */

bool A_transmission_ready;
float TIMEOUT = 11.0;
 
int sequence_number_A; // Sequence number being sent
int acknowlegde_number_A; 

int sequence_number_B; // Sequence number expected
int acknowledge_number_B; // next sequence nnumber from A

struct pkt packet_in_act;

int checksum_generator(pkt packet);
bool PacketCorrupted(pkt packet);

queue<msg> message_queue;

void A_output(struct msg message)
{ 	
	if(A_transmission_ready){ 
	//checking if A is ready to take a new message as an acknowledgement
	
		A_transmission_ready = false;
		// changing check to hold till acknowledgement is received

		packet_in_act = {};
		// Reset the current packet
		
		strncpy(packet_in_act.payload,message.data,sizeof(packet_in_act.payload)); 
		// Copying message  to current packet payload
		packet_in_act.seqnum = sequence_number_A;
		packet_in_act.acknum = acknowlegde_number_A;
		packet_in_act.checksum = 0;
		
		packet_in_act.checksum = checksum_generator(packet_in_act);
					
		tolayer3(0, packet_in_act);
		
		starttimer(0, TIMEOUT);
	}
	else{
		message_queue.push(message);		
	}	
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
	if(!PacketCorrupted(packet) && packet.acknum == sequence_number_A ){
	 //checking if packet is not corrupt 
		stoptimer(0);
		sequence_number_A = !sequence_number_A;
		
		if(!message_queue.empty()){
			msg next_msg = message_queue.front();
			message_queue.pop();
			A_transmission_ready = true;
			A_output(next_msg);	
		}
		else
			A_transmission_ready = true;	
	}	
}

/* called when A's timer goes off */
void A_timerinterrupt()
{ 
	A_transmission_ready = false;
	tolayer3(0, packet_in_act);
	starttimer(0, TIMEOUT);
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{	
	A_transmission_ready = true;
	sequence_number_A = 0;
	acknowlegde_number_A = 0;	
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{ 
		
	pkt ack_packet = {};
	
	 if(!PacketCorrupted(packet) && packet.seqnum == sequence_number_B){
		tolayer5(1, packet.payload);
		ack_packet.acknum = sequence_number_B;
		ack_packet.checksum = 0;
		ack_packet.checksum = checksum_generator(ack_packet);	 
		tolayer3(1, ack_packet);
		sequence_number_B = !sequence_number_B;
	}
	else if(!PacketCorrupted(packet) && packet.seqnum != sequence_number_B){
		ack_packet.acknum = !sequence_number_B;
		ack_packet.checksum = 0;
		ack_packet.checksum = checksum_generator(ack_packet);	 
		tolayer3(1, ack_packet);
	}
		
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
	sequence_number_B = 0;
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

bool PacketCorrupted(pkt packet){
	
	if(packet.checksum == checksum_generator(packet))
		return false;
	else
		return true;
		
}
