#include "dnsclient.h"
#include<stdio.h>
#include<stdlib.h>
#include<boost/array.hpp>
#include <iostream>
#define T_A 1 //Ipv4 address
#define T_NS 2 //Nameserver
#define T_CNAME 5 // canonical name
#define T_SOA 6 /* start of authority zone */
#define T_PTR 12 /* domain name pointer */
#define T_MX 15 //Mail server
#define T_NAPTR 35
struct DNS_HEADER
{
    unsigned short id; // identification number

    unsigned char rd :1; // recursion desired
    unsigned char tc :1; // truncated message
    unsigned char aa :1; // authoritive answer
    unsigned char opcode :4; // purpose of message
    unsigned char qr :1; // query/response flag

    unsigned char rcode :4; // response code
    unsigned char cd :1; // checking disabled
    unsigned char ad :1; // authenticated data
    unsigned char z :1; // its z! reserved
    unsigned char ra :1; // recursion available

    unsigned short q_count; // number of question entries
    unsigned short ans_count; // number of answer entries
    unsigned short auth_count; // number of authority entries
    unsigned short add_count; // number of resource entries
};
struct QUESTION
{
    unsigned short qtype;
    unsigned short qclass;
};
struct R_DATA
{
    unsigned short type;
    unsigned short _class;
    unsigned int ttl;
    unsigned short data_len;
    unsigned short other;
    unsigned short pref;
    unsigned short flagLen;
    unsigned char flag;
    unsigned short servLen;
    unsigned char serv;
    unsigned short regLen;
    unsigned char reg;
};
#pragma pack(pop)

//Pointers to resource record contents
struct RES_RECORD
{
    unsigned char *name;
    struct R_DATA *resource;
    unsigned char *rdata;
};
void DnsClient::ChangetoDnsNameFormat(char *dns, char *host){
    int lock = 0 , i;
        std::strcat(host,".");
        for(i = 0 ; i < strlen((char*)host) ; i++)
        {
            if(host[i]=='.')
            {
                *dns++ = i-lock;
                for(;lock<i;lock++)
                {
                    *dns++=host[lock];
                }
                lock++; //or lock=i+1;
            }
        }
        *dns++='\0';

}

u_char* DnsClient::ReadName(unsigned char* reader,unsigned char* buffer,int* count){
    unsigned char *name;
        unsigned int p=0,jumped=0;//,offset;
        *count = 1;
        name = (unsigned char*)malloc(256);
        name[0]='\0';
        //read the names in 3www6google3com format
        while(*reader!=0)
        {
            if(*reader>=192)
            {
                jumped = 1; //we have jumped to another location so counting wont go up!
            }
            else
            {
                name[p++]=*reader;
            }

            reader = reader+1;

            if(jumped==0)
            {
                *count = *count + 1; //if we havent jumped to another location then we can count up
            }
        }
        name[p]='\0'; //string complete
        if(jumped==1)
        {
            *count = *count + 1; //number of steps we actually moved forward in the packet
        }
        return name;
}

void DnsClient::do_send(char* number){
    boost::asio::ip::udp::endpoint endpoint(
    boost::asio::ip::address::from_string("8.8.8.8"), 53);
    dns = (struct DNS_HEADER *)&buf;
    dns->id = (unsigned short) htons(getpid());
    dns->qr = 0; //This is a query
    dns->opcode = 0; //This is a standard query
    dns->aa = 0; //Not Authoritative
    dns->tc = 0; //This message is not truncated
    dns->rd = 1; //Recursion Desired
    dns->ra = 0; //Recursion not available! hey we dont have it (lol)
    dns->z = 0;
    dns->ad = 0;
    dns->cd = 0;
    dns->rcode = 0;
    dns->q_count = htons(1); //we have only 1 question
    dns->ans_count = 0;
    dns->auth_count = 0;
    dns->add_count = 0;
    qname =(char*)&buf[sizeof(struct DNS_HEADER)];
    ChangetoDnsNameFormat(qname , number);
    qinfo =(struct QUESTION*)&buf[sizeof(struct DNS_HEADER) + (strlen((const char*)qname) + 1)];
    qinfo->qtype = htons( T_A );
    qinfo->qclass = htons(1);
    printf("\nSending Packet...");
    socket.async_send_to(boost::asio::buffer(buf,1024),
                         endpoint,boost::bind(&DnsClient::handle_send ,
                                              this ,
                                              boost::asio::placeholders::error()));
}

void DnsClient::do_receive(){

}

void DnsClient::handle_send(const boost::system::error_code &error){
    if(!error){
        printf("\nDone");
        printf("\nReceiving answer...");
        boost::asio::ip::udp::endpoint endpoint(
        boost::asio::ip::address::from_string("8.8.8.8"), 53);
        socket.async_receive_from(boost::asio::buffer(buf,1024),endpoint,
                                  boost::bind(&DnsClient::handle_receive,
                                  this , boost::asio::placeholders::error()));
    }else{
        printf("have some error");
        delete this;
    }
}

void DnsClient::handle_receive(const boost::system::error_code &error){
    if(!error){
        struct RES_RECORD answers[20],addit[20],auth[20];
        struct sockaddr_in a;
        int stop,i,j,s;
        dns = (struct DNS_HEADER*) buf;
        reader = &buf[sizeof(struct DNS_HEADER) + (strlen((const char*)qname)+1) + sizeof(struct QUESTION)];
        printf("\nThe response contains : ");
        printf("\n %d Questions.",ntohs(dns->q_count));
        printf("\n %d Answers.",ntohs(dns->ans_count));
        printf("\n %d Authoritative Servers.",ntohs(dns->auth_count));
        printf("\n %d Additional records.\n\n",ntohs(dns->add_count));
        stop=0;

        for(i=0;i<ntohs(dns->ans_count);i++)
            {
                answers[i].name=ReadName(reader,buf,&stop);
                reader = reader + stop;

                answers[i].resource = (struct R_DATA*)(reader);
                reader = reader + sizeof(struct R_DATA);

                if(ntohs(answers[i].resource->type) == T_NAPTR) //if its an ipv4 address
                {
                    answers[i].rdata = (unsigned char*)malloc(ntohs(answers[i].resource->data_len));
                    for(j=0 ; j<ntohs(answers[i].resource->data_len) ; j++)
                    {
                        answers[i].rdata[j]=reader[j];
                    }
                    answers[i].rdata[ntohs(answers[i].resource->data_len)] = '\0';
                    reader = reader + ntohs(answers[i].resource->data_len);
                }
                else
                {
                    answers[i].rdata = ReadName(reader,buf,&stop);
                    reader = reader + stop;
                }
            }
            //print answers
            printf("\nAnswer Records : %d \n" , ntohs(dns->ans_count) );
            for(i=0 ; i < ntohs(dns->ans_count) ; i++)
            {
               // printf("Name : %s ",answers[i].name);
                if(ntohs(answers[i].resource->type)==T_NAPTR)
                {
                    //Canonical name for an alias
                    printf("Regex : %s",answers[i].rdata);
                    char* ident;
                    ident=(char*)answers[i].rdata+(strlen((char*)answers[i].rdata)-3);
                    char* mtsIdent="01!";
                    if((strcmp(ident,mtsIdent))==0){
                    printf("\n Abonent mts");
                    }
                }

                printf("\n");
            }
    }
else{
        delete this;
    }
}
