#include "../include/simulator.h"

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <cstring>
#include <vector>

using namespace std;

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

Unidirectional data transfer 
   protocols (from A to B). Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

float TIMEOUT = 25.0;

int sequence_number_A; 
int acknowlegde_number_A; 
int window_size_A;
int send_b;
int next_sequence_number;

int expected_sequence_B; 

vector <msg> msg_buffer;

void send_data();
int checksum_generator(pkt packet);
bool corrupt_pkt(pkt packet);

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{

	//Message is added to the queue
	msg_buffer.push_back(message);
	
	// The sending routine is started
	send_data();
}

void send_data(){

	while(next_sequence_number < send_b + window_size_A && next_sequence_number<msg_buffer.size()){
		msg message = msg_buffer[next_sequence_number];
		// Creation of Packets 
		pkt packet;
		
		strncpy(packet.payload,message.data,sizeof(packet.payload)); 
		// Copying message to current packet payload
 		packet.seqnum = next_sequence_number;
		packet.acknum = acknowlegde_number_A;
		packet.checksum = 0;
		packet.checksum = checksum_generator(packet);
		tolayer3(0, packet);
		
		if(send_b == next_sequence_number){
			starttimer(0, TIMEOUT);
		} 
		next_sequence_number++;
		}
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{

 	if(!corrupt_pkt(packet)){
		send_b = packet.acknum +1;
		if(send_b == next_sequence_number)
			stoptimer(0);
		else{
			stoptimer(0);
			starttimer(0, TIMEOUT);
		}
 	}
}

/* called when A's TIMEOUT goes off */
void A_timerinterrupt()
{ 
	next_sequence_number = send_b;
	send_data();
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
	sequence_number_A = 0; 
	acknowlegde_number_A = 0; 
	window_size_A = getwinsize();
	send_b = 0;
	next_sequence_number = 0;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{ 
	if(!corrupt_pkt(packet) && expected_sequence_B == packet.seqnum){
	// adding packet corruption check
		tolayer5(1, packet.payload);
		pkt received_acknowledgement;
		received_acknowledgement.acknum = expected_sequence_B;
		received_acknowledgement.checksum = 0;
		received_acknowledgement.checksum = checksum_generator(received_acknowledgement);
		tolayer3(1, received_acknowledgement);	
		expected_sequence_B++;	
	}
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
	expected_sequence_B = 0;
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