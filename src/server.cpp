#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pqxx/pqxx>
#include "tinyxml2.h"
#include "handle.h"
#include "db.h"
#include "ThreadPool.h"

using namespace std;
using namespace pqxx;
using namespace tinyxml2;

#define THREAD_POOL 0

int main(int argc, char *argv[])
{
  int status;
  int socket_fd;
  struct addrinfo host_info;
  struct addrinfo *host_info_list;
  const char *hostname = NULL;
  const char *port = "12345";

  memset(&host_info, 0, sizeof(host_info));

  host_info.ai_family   = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;
  host_info.ai_flags    = AI_PASSIVE;

  status = getaddrinfo(hostname, port, &host_info, &host_info_list);
  
  if (status != 0) {
    cerr << "Error: cannot get address info for host" << endl;
    return EXIT_FAILURE;
  } 

  //create socket
  socket_fd = socket(host_info_list->ai_family, 
		     host_info_list->ai_socktype, 
		     host_info_list->ai_protocol);
  if (socket_fd == -1) {
    cerr << "Error: cannot create socket" << endl;
    return EXIT_FAILURE;
  }

  //reuse port
  int yes = 1;
  status = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  //bind socket with info
  status = bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    cerr << "Error: cannot bind socket" << endl;
    return EXIT_FAILURE;
  }

  //set listening limit
  status = listen(socket_fd, 1000);
  if (status == -1) {
    cerr << "Error: cannot listen on socket" << endl; 
    return EXIT_FAILURE;
  } 

  //set blocked connection
  cout << "Waiting for connection on port " << port << endl;
  struct sockaddr_storage socket_addr; 
  socklen_t socket_addr_len = sizeof(socket_addr);

  //===========================================================================
  //database
  connection *C;
  
  try
  {
      C = new connection("dbname=exchange_db user=postgres password=passw0rd host=db port=5432");
      if (C->is_open())
      {
        cout << "Opened database successfully: " << C->dbname() << endl;
      }
      else
      {
        cout << "Can't open database" << endl;
        return 1;
      }
  }
  catch (const std::exception &e)
  {
    cerr << e.what() << std::endl;
    return 1;
  } 

  //initialize the database
  dropTable(C, "OPEN_ORDER");
  dropTable(C, "EXEC_ORDER");
  dropTable(C, "CANCEL_ORDER");
  dropTable(C, "POSITION");
  dropTable(C, "ACCOUNT");
  createTables(C);

  // int pool_size = atoi(argv[1]);

  #if THREAD_POOL
    threadpool pool{20};
  #endif

  while(1){
       try{
          cout << "accepting..." << endl;
	        int client_connection_fd = accept(socket_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
          if (client_connection_fd == -1) {
            cerr << "Error: cannot accept connection on socket" << endl;
            return EXIT_FAILURE;
          } 

          //handle the request XML
          //Add ThreadPool later
          //handle_XML(C, client_connection_fd);
          
          #if THREAD_POOL
            pool.commit(handle_XML, C, client_connection_fd).get();
          #else
            thread t(handle_XML, C, client_connection_fd);
            t.join();
          #endif
       }
       catch(exception & excp){
          cerr << excp.what() << endl;
          continue;
       }
    }

  freeaddrinfo(host_info_list);
  close(socket_fd);

  C->disconnect();

  return 0;
}
