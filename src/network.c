#include "./network.h"
#include "./serilaization.h"
#include "block.h"
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
// typedef struct Network_Block{
//     int version;
//     int content_len ;
//     enum Message message_type;
//     Block *data;
// }Network_Block ;



P2P_network *init_p2p(){
    P2P_network *p2p  = (P2P_network *) malloc(sizeof(P2P_network));
    p2p->connecd_nodes_number = 0;
    return p2p;
}

// i should make sure that the packet contains data before serilize it.
Buffer *serilized_network(Network_Block *packet){
    // Here we allocate dynamic twice one for the struct it self and \
    // one for the char pointer within the struct
    Buffer *buf = (Buffer *)malloc(sizeof(Buffer));
    int buffer_len = (sizeof(int) * 2) + sizeof(packet->message_type) + packet->content_len;

    buf->buffer = (unsigned char *)malloc(buffer_len);
    buf->size = buffer_len;
    
    unsigned char *ptr = buf->buffer;
    memcpy(ptr, &packet->version, sizeof(int));
    ptr+= sizeof(int);
    memcpy(ptr, &packet->content_len, sizeof(int));
    ptr+= sizeof(int);
    memcpy(ptr, &packet->message_type, sizeof(packet->message_type));
    ptr+= sizeof(packet->message_type);
    memcpy(ptr,  serilized(packet->data) , packet->content_len);
    return buf;
}

Network_Block *deserilized_network(Buffer *buffer){
    Network_Block *packet = (Network_Block *)malloc(sizeof(Network_Block));
        unsigned char *ptr = buffer->buffer;

        memcpy(&packet->version, ptr, sizeof(int));
        ptr += sizeof(int);
        memcpy(&packet->content_len, ptr, sizeof(int));
        ptr += sizeof(int);
        memcpy(&packet->message_type, ptr, sizeof(packet->message_type));
        ptr += sizeof(packet->message_type);

        if (packet->content_len > 0) {
            // Allocate memory for data_buffer with error checking
            unsigned char *data_buffer = (unsigned char *)malloc(packet->content_len);
            if (data_buffer == NULL) {
                free(packet);
                return NULL;
            }

            // Copy data, ensuring it doesn't exceed buffer size
            memcpy(data_buffer, ptr, packet->content_len);

            // Deserialize the Block (data field)
            packet->data = deserilized(data_buffer);
            free(data_buffer);
        } else {
            packet->data = NULL;
        }

        return packet;
}

void print_packet(Network_Block *packet){
    printf("version : %i\n", packet->version);
    printf("content len : %i\n", packet->content_len);
    printf("message type : %i\n", packet->message_type);
    print_block(packet->data);
    printf("\n");
}

// Create a function to create a socket
Node *socket_gen(int lport){

    Node *node = malloc(sizeof(Node));
    if(node == NULL){
        perror("Error while allocating memory for the node");
        exit(MEMORY_ALLOCATION_ERROR);
    }
    node->socket_length = sizeof(node->address_info);
    if(lport > 65535 || lport < 0 ){
        node->common_port = DEFAULT_PORT_NUMBER_FOR_SOCKET;
    }else{
        node->common_port = lport;
    }

    if ((node->socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        node->socket = CREATE_SOCKET_ERROR;
        perror("Error while createing socket");
        exit(CREATE_SOCKET_ERROR);
    }
    node->address_info.sin_addr.s_addr = INADDR_ANY;
    node->address_info.sin_port = htons(node->common_port);
    node->address_info.sin_family = AF_INET;
    memset(&node->address_info.sin_zero, 0, sizeof(node->address_info.sin_zero));

    if(bind(node->socket, (struct sockaddr *) &node->address_info, node->socket_length) < 0){
        perror("Error while binding");
        exit(BIND_SOCKET_ERROR);
    }
    return node;
}


// SO this function will set up a listener on a common port for nodes that want to connect.
void make_socket_listen(Node *node, int backlog, P2P_network *p2p){
    // This represnt the new connected node
    struct sockaddr_in next_node_tmp;
    int client;
    socklen_t socket_length = sizeof(next_node_tmp);
    if(node == NULL){
        perror("node is null can not listen");
        exit(NULL_NODE);
    }

    if(listen(node->socket, backlog) < 0){
        perror("Error while listen");
        exit(LISTEN_SOCKET_ERROR);
    }
    printf("the Socket listen on %s:%i\n",inet_ntoa(node->address_info.sin_addr) , ntohs(node->common_port));
    // This should be in a single thread where this thread will update the node->nodes array and
    // hopfully we are able to access it later so that we can broadcast a message.
    while (1){
        // This will return the file discriptor of the new connected node.
        if((client = accept(node->socket, (struct sockaddr *) &next_node_tmp, &socket_length)) < 0){
            Connected_Node_Info *new_node = malloc(sizeof(Connected_Node_Info));
            new_node->fd = client;
            new_node->conn_node = next_node_tmp;
            // The array will take an address refres to the new connected nodes.
            p2p->nodes[p2p->connecd_nodes_number++] = new_node;
            // Next a new thread should be created an move this data to that thread;
        };
    }
}



// Since we are serilizing the Network block message we just
// need to get the buffer thats hold the value
// Also its a good idea to free the unsigned char buffer;
void broadcast(Node *node, Buffer *serilized_message, P2P_network *p2p){
    if(node == NULL){
        perror("node is null can not broadcast the block");
        exit(NULL_NODE);
    }

    for(int i = 0; i < p2p->connecd_nodes_number ; i++){
        // TODO: the size of the buffer i have not calculatd yet it should be input too for the funtion
        // For now we just gonna stick with this.
        int number_of_sent_bytes = send(p2p->nodes[i]->fd, serilized_message->buffer, serilized_message->size, 0);
    }
    printf("The block has been broadcast");
    free(serilized_message);
}



// I actually make it easy for my self since my p2p->nodes array holds the other node
// inforamtion i can just refres to each other node IP in order to connect to them.
void connecter(){
    // TODO: First am thinking of making a function that listen on some port \
    // and this should be common between all nodes so they can find themself on that port.
}

// Make this function so that it send a broadcast message on UDP \ 
// When other node receives this message they replay with their IP \
// So we can establish TCP connection. and also we need to validate the node\
// so that we can be sure that this node is not malicious
//  Why UDP, As you may know TCP does not support broadcast by nature since they are \
//  connectionful network protocol. you can referes to broadcasr function to see how we can \
//  send broadcast via TCP.
// Refrence : https://cs.baylor.edu/~donahoo/practical/CSockets/code/BroadcastSender.c
// https://cs.baylor.edu/~donahoo/practical/CSockets/code/BroadcastReceiver.c
void discover_nodes(){

}
