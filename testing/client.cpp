#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <vector>
#include <sstream>
#include <fstream>
#include <time.h> 

using namespace std;

double calculateRuntime(struct timespec timeStart, struct timespec timeEnd){
  double runtime = 0; 
  runtime = (timeEnd.tv_sec - timeStart.tv_sec)*1000000000 + (double)(timeEnd.tv_nsec -timeStart.tv_nsec);
  return runtime;
}

void * clientFunc(void *arg)
{
  int status;
  int socket_fd;
  struct addrinfo host_info;
  struct addrinfo *host_info_list;
  const char *hostname = "vcm-26203.vm.duke.edu";  //needs to be changed later
  const char *port = "12345";
  
  struct hostent * server_info = gethostbyname(hostname);

  if (server_info == NULL) {
      cout << "Syntax: client <hostname>\n" << endl;
      //return 1;
      exit(1);
  }

  memset(&host_info, 0, sizeof(host_info));
  host_info.ai_family   = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;

  status = getaddrinfo(hostname, port, &host_info, &host_info_list);
  if (status != 0) {
    cerr << "Error: cannot get address info for host" << endl;
    //return -1;
    exit(1);
  }

  //create socket
  socket_fd = socket(host_info_list->ai_family, 
		     host_info_list->ai_socktype, 
		     host_info_list->ai_protocol);
  if (socket_fd == -1) {
    cerr << "Error: cannot create socket" << endl;
    //return -1;
    exit(1);
  } 
  
  //cout << "Connecting to " << hostname << " on port " << port << "..." << endl;
  
  //connect
  status = connect(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    cerr << "Error: cannot connect to socket" << endl;
    //return -1;
    exit(1);
  }

  //send message
  ifstream inFile;
  char line[1024] = {0};
  string filename = (char *)arg;
  inFile.open(filename, ios::in);
  //int xml_length = 0;
  stringstream ss_xml;
  if(inFile){
      //cout << "XML file opened successfully" << endl;

      while(inFile.getline(line, sizeof(line))){
          ss_xml << line;
      }
      inFile.close();
  }
  else{
      cout << "Can't open XML file." << endl;
      //return EXIT_FAILURE;
      exit(1);
  }

  string s_xml = ss_xml.str();
  int xml_length = s_xml.size();
  //cout << "s_xml: " << s_xml << endl;
  vector <char> v_xml;
  v_xml.resize(xml_length);
  v_xml.assign(s_xml.begin(),s_xml.end());

  //send the length of xml
  send(socket_fd, &xml_length, sizeof(int), 0);
  
  //send the text of xml
  send(socket_fd, &(v_xml[0]), xml_length, 0);

  //receive the length of xml
  int response_length = 0;
  recv(socket_fd, &response_length, sizeof(int), 0);
  //cout << "response_length: " << response_length << endl;
  
  //receive the text of xml
  char response_text[response_length+1] = {0};
  recv(socket_fd, response_text, response_length, 0);
  cout << "===========response_text: \n" << response_text << endl;

  freeaddrinfo(host_info_list);
  close(socket_fd);

  return EXIT_SUCCESS;
}

int main(int argc, char ** argv) {
  struct timespec timeStart, timeEnd;
  clock_gettime(CLOCK_REALTIME,&timeStart);
  
  if(argc==2){
    clientFunc(argv[1]);
  }

  if(argc==3){
    int MAX_THREAD = atoi(argv[2]);
    pthread_t thread_ids[MAX_THREAD];

    for (int i = 0; i < MAX_THREAD; i++) {
      pthread_create(&thread_ids[i], NULL, clientFunc, argv[1]);
      usleep(1000);
      pthread_join(thread_ids[i], NULL);
    }
  }

  clock_gettime(CLOCK_REALTIME,&timeEnd);
  double runtime = calculateRuntime(timeStart, timeEnd);
  cout << "===========runtime: " << runtime << endl;

  return 0;
}