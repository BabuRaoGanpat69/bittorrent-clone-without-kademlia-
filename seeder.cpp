#include <algorithm>
#include <arpa/inet.h>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <mutex> //give errors ignoring hard code and error flags also keep this in mind that this is not true bittorrent and i am running this on my local machine
#include <netinet/in.h>
#include <openssl/sha.h>
#include <random> // for rand
#include <shared_mutex>
#include <sstream> // for stringstream
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>
using namespace std;
using namespace std::chrono; // g++ -std=c++17 -g peer1.cpp -o peer1 -lssl -lcrypto -pthread
                             //
#include <nlohmann/json.hpp> //g++ -std=c++17 -g seeder.cpp -o seeder -lssl -lcrypto -pthread
using json = nlohmann::json;
// std::shared_mutex mtxforsend;
std::shared_mutex mtxforsendrecv;
std::shared_mutex mtxforconnected_fds_map_con;
std::shared_mutex mtxforallpiece_info;
std::shared_mutex mtxforpieces_ihave_ooyaehhh;
std::shared_mutex mtxforallinterested;
std::shared_mutex mtxforallchoked;
std::shared_mutex mtxforpeer_speed;
std::shared_mutex mtxforfor_raresandgetrightidt;
std::shared_mutex mtxforforchoke;
std::shared_mutex mtxforpiece_block_map;
std::shared_mutex mtxforfirstfive; // this is used to block choke and unchoke messages
/// std::shared_mutex mtxforblockingchoke;
uint32_t rarest = 0;             // rarest piece
uint32_t my_port = 8080;         // my port
const char *my_ip = "127.0.0.1"; // my ip
const char *s_hash;              // fxn does not work with string thats why
string file_hash_string;         // justified
string info_hash;
const char *info_hash_char;
const char *file_hash;
uint32_t piece_length_global;     // length of single piece
uint32_t num_pieces;              // number of pieces
uint32_t max_message_size = 8300; // max message size

vector<uint32_t> interested; // justified
vector<uint32_t> I_interested;
vector<uint32_t> uninterested; // justified
vector<uint32_t> I_uninterested;
vector<uint32_t> chocked_by_me;         // justified
vector<uint32_t> chocked_by;            // justified
vector<uint32_t> unchocked_by;          // justified
vector<uint32_t> unchocked_by_me;       // justified
vector<uint32_t> seeders{8080};         // seeder hardcoded
vector<uint32_t> for_rarest;            // justified
vector<uint32_t> sorted_arry;           // arry of interested speed
vector<uint32_t> pieces_ihave_dont;     // justified
vector<uint32_t> pieces_ihave_ooyaehhh; // pieces i have
vector<char> s_hash_vec;                // tem vector to store string
vector<uint32_t> no_use{0, 0, 0, 0, 0, 0,
                        0, 0, 0, 0, 0}; // 10 peers will send spped
vector<uint32_t> ports;
vector<uint32_t> temport;
vector<uint32_t> tempspeed;
uint8_t snd_choke = 5;        // size of protocol message
uint8_t snd_unchoke = 5;      // size of protocol message
uint8_t snd_inerested = 5;    // size of protocol message
uint8_t snd_notinerested = 5; // size of protocol message
uint8_t snd_have = 9;         // size of protocol message
uint8_t snd_request = 17;     // size of protocol message
// uint16_t snd_piece = 8205;    // size of protocol message
uint32_t snd_cancel = 17;   // size of protocol message
uint32_t snd_hanshake = 25; // size of protocol message
uint32_t unique_piece;      // checks if the file is downloaded yet
uint32_t firstfive[5];      // first five random pieces
uint32_t totalfilesize;
uint32_t unique_block_len_of_unique_pi = 0;
uint32_t no_of_block_in_normal_piece = 0;
uint32_t no_of_block_in_unique_piece = 0;
uint32_t unique_block_len_of_normal = 0;
bool IamSeeder, ogseeder = true;             // am i seeder?
bool other, downloaded_yet, first_5 = false; // size of the unique piece
int totalfilesize_int = 0;                   // totl size of file i am sending
int piece_length_int;
int getrightid;
map<uint32_t, uint32_t> connected_fds_map_con; // port is key and fd used by it is value
map<uint32_t, uint32_t> port_speed;            // map of interested port and the speed
// map<uint32_t, uint32_t> peer_info;                  // trcaks piece info
map<uint32_t, vector<uint32_t>> piece_info;         // trackes pieces taht pers have used for finfign rarest piece
map<string, map<int, string>> file_info;            // justified
map<uint32_t, uint16_t> block_count;                // taki agr fxn firse call krne ho to
map<uint32_t, vector<uint32_t>> piece_info_reverse; // key is port and i see the pieces that port have
map<uint32_t, uint32_t> piece_info_vec;
map<uint32_t, uint32_t> peer_speed; // trackes speed wtih key being speed and value port
map<uint32_t, vector<uint32_t>> piece_block_map;
string piece_hash_string;
const uint32_t block_len_global = 8192; // default block len 8 kilobytes//this shouldnit beconst
// call fo rmy socket fxn
class quick_sort // sorting algorithm
{
public:
  void swap(uint32_t *x,
            uint32_t *y) // make sure both vector have same size nhi to lode lga
  {
    uint32_t temp_swap = *x;
    *x = *y;

    *y = temp_swap;
  }
  void quicksort(vector<uint32_t> &sorted_arry1, uint32_t length, vector<uint32_t> &sorted_temp)
  {
    cout << "checking if sort can work" << endl;
    while (sorted_arry1.empty() && sorted_temp.empty())
    {
      sleep(1);
      cout << "size" << endl;
    }
    cout << "check passed" << endl;
    sorted_temp.resize(sorted_arry1.size());
    quicksort_recursion(sorted_arry1, 0, length - 1, sorted_temp);
  }
  void quicksort_recursion(vector<uint32_t> &sorted_arry1, int low, int high, vector<uint32_t> &sorted_temp)
  {
    if (low < high)
    {
      int pivot_index = partition(sorted_arry1, low, high, sorted_temp);
      quicksort_recursion(sorted_arry1, low, pivot_index - 1, sorted_temp);
      quicksort_recursion(sorted_arry1, pivot_index + 1, high, sorted_temp);
    }
  }
  int partition(vector<uint32_t> &sorted_arry1, int low, int high, vector<uint32_t> &sorted_temp)
  {
    cout << high << endl;
    uint32_t pivot_value = sorted_arry1[high];
    int i = low;
    for (int j = low; j < high; j++)
    {
      if (*(sorted_arry1.data() + j) >= pivot_value)
      {
        swap(&sorted_arry1[i], &sorted_arry1[j]);
        swap(&sorted_temp[i], &sorted_temp[j]);
        i++;
      }
    }
    swap(&sorted_arry1[i], &sorted_arry1[high]);
    swap(&sorted_temp[i], &sorted_temp[high]);

    return i;
  }
};
class my_jason // for reading teh torrent(currently using jason inplace of
               // torrent ) , for info hash check, and piece hash check)
{
public:
  json j;
  uint32_t piece_length;

  void my_jason1()
  {
    fstream inputfile;
    inputfile.open("/home/kartik/Documents/codecpp/hk.json", ios::in | ios::binary);
    if (inputfile.is_open())
    {
      inputfile >> j;
      info_hash = j["info"].dump(0);
      info_hash_char = info_hash.c_str();
      piece_length_int = j["info"]["piece length"].get<int>();
      piece_hash_string = j["info"]["pieces"].get<string>();
      cout << "got hash" << piece_hash_string.size();
      cout << " piece length int is " << piece_length_int << endl;
      piece_length_global = (uint32_t)piece_length_int;
      piece_length = piece_length_global;
      string full_path;
      for (auto &extension : j["info"]["files"]) // arry of all
      {
        full_path = "/";
        if (extension["path"].is_array())
        {
          for (auto &gbd : extension["path"]) // path
          {
            // cout << full_path << endl;
            full_path = full_path + gbd.get<string>();
            cout << " name file" << full_path << endl;
          }
        }
        else
        {
          full_path = extension["path"].get<string>();
          cout << " name file" << full_path << endl;
        }
        file_info[full_path][extension["length"].get<int>()] =
            extension["sha1"].get<string>();
        totalfilesize_int += extension["length"].get<int>();
      }
      totalfilesize = (uint32_t)totalfilesize_int;
      if (totalfilesize % piece_length_global == 0)
      {
        num_pieces = totalfilesize / piece_length_global;
        unique_piece = piece_length_global;
      }
      else
      {
        num_pieces = ((totalfilesize - (totalfilesize % piece_length_global)) / piece_length_global) + 1; // ok
        unique_piece = totalfilesize % piece_length_global;
      }
      if (piece_length_global % block_len_global != 0)
      {
        unique_block_len_of_normal = piece_length_global % block_len_global;
        no_of_block_in_normal_piece = (piece_length_global - unique_block_len_of_normal) / block_len_global + 1;
      }
      else
      {
        unique_block_len_of_normal = block_len_global;
        no_of_block_in_normal_piece = piece_length_global / block_len_global;
      }
      if (unique_piece % block_len_global != 0)
      {
        unique_block_len_of_unique_pi = unique_piece % block_len_global;
        no_of_block_in_unique_piece = (unique_piece - unique_block_len_of_unique_pi) / block_len_global + 1;
      }
      else
      {
        unique_block_len_of_unique_pi = block_len_global;
        no_of_block_in_unique_piece = unique_piece / block_len_global;
      }
    }
    else
    {
      throw std::runtime_error("torent file not dumped in jason sad");
    };
    cout << "got total file size" << totalfilesize << endl;
    cout << num_pieces << "numpieces" << endl;
    inputfile.close();
  }
  bool extension_giver_cum_piecehash(const uint32_t &pieceid_hash) // wil be checking for piece hash to chech                               // if piece is balid
  {
    char filedataforhash[20];
    char piece_hash_torrent[20]; // hahs i get from torrent
                                 // Check the actual type
    cout << j["info"]["pieces"].type_name() << endl;

    string byte;
    string final; // string containing all hash i string form
    const char *final_char;
    int tempfilelength;
    uint32_t tempfilelength_uint32_t;
    cout << "checking piece hash of " << pieceid_hash << " whole piece json obj" << piece_hash_string.size() << endl;
    stringstream gethash(piece_hash_string.substr(5));
    while (gethash >> byte)
    {
      final += byte;
    }

    final_char = final.c_str();
    memcpy(piece_hash_torrent, final_char + 20 * pieceid_hash, 20); // getting pieces_hashthe desired piece hash
                                                                    // string tocheck(final,20);
                                                                    // cout << "hash is " << final << endl;
    fstream hashfile;                                               // add flags in future to check if piece is even there or not
    hashfile.open("/home/kartik/Downloads/" + to_string(my_port) + "/" + to_string(pieceid_hash) + ".temp", ios::in | ios::binary);
    if (pieceid_hash != num_pieces)
    {
      tempfilelength = (int)piece_length_global;
    }
    else
    {
      tempfilelength = (int)unique_piece;
    }
    tempfilelength_uint32_t = (uint32_t)tempfilelength;
    char *filedatahash = new char[tempfilelength];
    if (hashfile.is_open())
    {
      hashfile.read(filedatahash, tempfilelength);
    }
    string data_str(filedatahash, tempfilelength_uint32_t);
    string piece_hash_me_string = sha1_calculator(data_str, tempfilelength_uint32_t);
    // cout << "hash i calculated" << piece_hash_me_string << endl;

    delete[] filedatahash;
    const char *piece_hash_me_char = piece_hash_me_string.c_str();
    cout << "checking pices hash of " << pieceid_hash << endl;
    if (memcmp(piece_hash_torrent, piece_hash_me_char, 20) == 0)
    {
      cout << "hash file check passed by" << pieceid_hash << endl;
      return true;
    }
    else
    {
      cout << "hash file check failed by" << pieceid_hash << endl;
      return false;
    }
  }
  string sha1_calculator(const string &data,
                         uint32_t size_hash) // calculates hash
  {
    unsigned char hash[SHA_DIGEST_LENGTH];
    string final_string;
    SHA1(reinterpret_cast<const unsigned char *>(data.c_str()), size_hash,
         hash);
    stringstream ss;
    // unchocked_by_me;
    for (int i = 0; i < SHA_DIGEST_LENGTH; ++i)
      ss << hex << setw(2) << setfill('0') << (int)hash[i];
    final_string = ss.str();
    std::transform(final_string.begin(), final_string.end(),
                   final_string.begin(),
                   [](unsigned char c)
                   { return std::towupper(c); });

    return final_string;
  };
};
class connectionhandler // contains send fxn
{
public:
  uint8_t loop_back1 = 0;
  quick_sort forclassch;
  void sendfxn_piece(uint32_t length_prefix_ho, uint8_t message_id,
                     uint32_t piece_id_ho, uint32_t offset_ho, uint32_t length_ho, const uint32_t connectedfd)
  {
    if (length_prefix_ho < 0 || length_prefix_ho > max_message_size || message_id < 0 || message_id > 15)
    {
      throw std::runtime_error("Invalid length prefix or message ID at sendfxnn_piece");
    }

    length_prefix_ho = length_ho + 13;
    uint8_t *send_arry = new uint8_t[length_ho + 17];
    uint32_t piece_id_no = htonl(piece_id_ho);
    uint32_t offset_no = htonl(offset_ho);
    uint32_t length_no = htonl(length_ho);

    //  message_id = htonl(message_id);
    uint32_t length_prefix_no = htonl(length_prefix_ho);

    thread([=]()
           {
     // std::unique_lock<std::shared_mutex> sr67(mtxforsendrecv);
     cout << "sendfxn_piece_called" << "LP" << length_prefix_ho << "  PD  " << piece_id_ho << " OFFSET" << offset_ho << " l " << length_ho << endl;
   
     // std::unique_ptr<uint8_t[]> send_arry = std::make_unique<uint8_t[]>(length_prefix + 4);
     int send_finished = 0;
     int send_current = 0; // this int as it can give -1 for not working ok so
                          // use int no unsigned int
     memcpy(send_arry, &length_prefix_no, 4);
     memcpy(send_arry + 4, &message_id, 1);
     memcpy(send_arry + 5, &piece_id_no, 4);
    memcpy(send_arry + 9, &offset_no, 4);
    memcpy(send_arry + 13, &length_no, 4);
    auto start = std::chrono::high_resolution_clock::now();
    fstream sendfile;
    sendfile.open("/home/kartik/Downloads/" + to_string(my_port) + "/" + to_string(piece_id_ho) + ".temp", ios::in | ios::binary);
    if (sendfile.is_open())
    {
      cout << "opened file" << "/home/kartik/Downloads/" + to_string(my_port) + "/" + to_string(piece_id_ho) + ".temp" << "  length " << length_ho  << "  length_prefix" << length_prefix_ho << endl;

      // sendfile.read(reinterpret_cast<char *>(send_arry + 17), ntohl(length));
      sendfile.seekg(offset_ho);
      if (sendfile.fail())
      {
        std::cerr << "Failed to seek in file for piece " << piece_id_ho << endl;
        // delete[] data;
        // delete[] temp_buffer;
        return;
      }
      sendfile.read(reinterpret_cast<char *>(send_arry + 17),length_ho);
      if (sendfile.fail())
      {
        std::cerr << "Failed to write data to file for piece " << piece_id_ho << endl;
      }
    }
    else if (loop_back1 < 2)
    {
      cout << "send failed trying agin" << endl;
      loop_back1++;
     // length_prefix = ntohl(length_prefix);
     // piece_id = ntohl(piece_id);
      //offset = ntohl(offset);
     // length = ntohl(length);
      sendfxn_piece(length_prefix_no, message_id, piece_id_no, offset_no, length_no, connectedfd);
    }
    sendfile.close();
   // length = ntohl(length); // convert length back to host byte order
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_seconds_forread = end - start;
    auto start1 = std::chrono::high_resolution_clock::now();
    while (send_finished < int(length_ho + 17))
    {
      send_current = send(connectedfd, send_arry + send_finished, length_ho + 17 - send_finished, 0);
      if (send_current == -1)
      {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
          continue;
        }
        std::cerr << "sendfxn_piece send failed: " << strerror(errno) << endl;
        break;
      }
      else if (send_current == 0)
      {
        std::cerr << "sendfxn_piece send failed: sent 0 bytes" << endl;
        break;
      }
      send_finished = send_current + send_finished;
      cout << "sendfxn_piece send finished" << send_finished << endl;
    }
    auto end1 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_seconds_forsend = end1 - start1;
    cout << "sendfxn_piece called" << "lemngth" << send_finished << " time for read " << elapsed_seconds_forread.count() << " time for send " << elapsed_seconds_forsend.count()<< endl;
    delete[] send_arry; })
        .detach();
  };
  void sendfxn_normal(uint8_t message_id_normal,
                      uint32_t connectedfd) // for choke and all
  {
    if (message_id_normal < 0 || message_id_normal > 15)
    {
      throw std::runtime_error("Invalid length prefix or message ID at normal");
    }
    message_id_normal = int(message_id_normal);
    thread([=]
           {
    cout << "sendfxn_nroaml called ta line 369  " << int(message_id_normal) << "  fd  " << connectedfd << endl;
    int error = 0;
    socklen_t len = sizeof(error);
    int result = getsockopt(connectedfd, SOL_SOCKET, SO_ERROR, &error, &len);

    if (result != 0)
    {
      cout << "getsockopt failed: " << strerror(errno) << endl;
      return;
    }
    if (error != 0)
    {
      cout << "Socket has error: " << strerror(error) << endl;
      return;
    }
    cout << "Socket fd " << connectedfd << " appears healthy" << endl;

    uint8_t send_arry[5];
    int send_finished = 0;
    int send_current = 0; // this int as it can give -1 for not working ok so
                          // use int no unsigned int
    uint32_t length_prefix_normal = 1;
    int error_check = 0;
    length_prefix_normal = htonl(length_prefix_normal);
    memcpy(send_arry, &length_prefix_normal, 4);
    memcpy(send_arry + 4, &message_id_normal, 1);
    cout << "stepped into mtxsndrecv sr2 " << endl;
    while (send_finished < 5)
    {
      send_current = send(connectedfd, send_arry + send_finished, 5 - send_finished, 0);
      if (send_current == -1)
      {
        cout << "sendfxn_normal send failed" << endl;
        break;
      }
      else if (send_current == 0)
      {
        cout << "sent sendfxn_nomal failed" << endl;
        break;
      };
      send_finished = send_current + send_finished;
      // cout << "sendfxn_normal send finished" << send_finished << endl;
    }
    cout << "&[sendfxn_normal sent lemngth prefix]& " << ntohl(length_prefix_normal) << " &" << int(message_id_normal) << endl; })
        .detach();
    // delete[] send_arry;
  };
  void sendfxn_req(uint32_t piece_id, uint32_t offset, uint32_t length, uint32_t connectedfd)
  {
    if (offset < 0 || length < 0 || piece_id < 0 || connectedfd < 0 || length > max_message_size)
    {
      throw std::runtime_error("Invalid length prefix or message ID at send req");
    }
    piece_id = htonl(piece_id);
    offset = htonl(offset);
    length = htonl(length);
    thread([=]()
           {
      // std::shared_lock<std::shared_mutex> sr3(mtxforsend);
      cout << "sendfxn_req_called" << "  PD  " << piece_id << " OFFSET" << offset << " l " << length << endl;
      uint32_t send_state = 17;
      uint8_t message_id = 6;
      uint32_t length_prefix = 13;
      uint8_t *send_arry = new uint8_t[17];
      int send_finished = 0;
      int send_current = 0; // this int as it can give -1 for not working ok so use int no
      // unsigned int length_prefix = htonl(length_prefix);

      length_prefix = htonl(length_prefix);
      memcpy(send_arry, &length_prefix, 4);
      memcpy(send_arry + 4, &message_id, 1);
      memcpy(send_arry + 5, &piece_id, 4);
      memcpy(send_arry + 9, &offset, 4);
      memcpy(send_arry + 13, &length, 4);
      while (send_finished < int(send_state))
      {
        send_current = send(connectedfd, send_arry + send_finished,
                            send_state - send_finished, 0);
        if (send_current == -1)
        {
          if (errno == EAGAIN || errno == EWOULDBLOCK)
          {
            continue;
          }
          std::cerr << "sendfxn_req send failed: " << strerror(errno) << endl;
          break;
        }
        else if (send_current == 0)
        {
          std::cerr << "sendfxn_req send failed: sent 0 bytes" << endl;
          break;
        }
        send_finished = send_current + send_finished;
        cout << "sendfxn_req send finished" << send_finished << endl;
      };
      cout << "sendfxn_req sent" << send_state << "lemngth prefix" << ntohl(length_prefix)
           << endl;
      delete[] send_arry; })
        .detach();
  }
};
class updater_cum_download // incoming messag ehandler
{
public:
  quick_sort qs; // to use quick sort// could have used inheritance
  connectionhandler forpublic;
  my_jason forhash; //""
  void rec_msg(uint32_t &length_prefix, uint32_t &connectedfd,
               uint8_t *temp_buffer, uint32_t &peer_port)
  {
    uint8_t message_id_uint = 0;
    // memcpy(&length_prefix, temp_buffer, 4);
    if (length_prefix > max_message_size)
    {
      cout << "cout retrning as length prefix is greater than max message size" << endl;
      delete[] temp_buffer;
      return;
    }
    memcpy(&message_id_uint, temp_buffer + 4, 1);
    int message_id = int(message_id_uint);
    cout << "lenght prefix rec_msg" << length_prefix << "  message_id  " << message_id << " fd  " << connectedfd << "port" << peer_port << endl;
    //  length_prefix = ntohl(length_prefix);
    //  cout << "lenght prefix rec_msg" << length_prefix << endl;

    cout << "messag id" << message_id << endl;
    if (message_id == 0)
    { // choke
      cout << "recevied choke by" << peer_port << "length prefix" << length_prefix << endl;
      std::unique_lock<std::shared_mutex> chocked_by7(mtxforallchoked);
      chocked_by.push_back(peer_port);
      unchocked_by.erase(
          std::remove(unchocked_by.begin(), unchocked_by.end(), peer_port),
          unchocked_by.end());
      delete[] temp_buffer;
    }
    else if (message_id == 1)
    { // unchoke
      cout << "recevied unchoke by" << peer_port << "length prefix" << (length_prefix) << endl;
      std::unique_lock<std::shared_mutex> mtxforunchocked_by89(mtxforallchoked);
      // std::unique_lock<std::shared_mutex>
      // mtxforinterested7(mtxforchocked_by);
      unchocked_by.push_back(peer_port);
      chocked_by.erase(
          std::remove(chocked_by.begin(), chocked_by.end(), peer_port),
          chocked_by.end());
      delete[] temp_buffer;
    }
    else if (message_id == 2)
    { // interested
      cout << "recevied interested by" << peer_port << "length prefix"
           << length_prefix << endl;
      std::unique_lock<std::shared_mutex> mtxforinterestedhh(
          mtxforallinterested);
      // std::unique_lock<std::shared_mutex>
      // mtxforuninterestedg(mtxforinterested);
      interested.push_back(peer_port);
      uninterested.erase(
          std::remove(uninterested.begin(), uninterested.end(), peer_port),
          uninterested.end());
      delete[] temp_buffer;
    }
    else if (
        message_id ==
        3)
    { // uninteresteduinterested.erase(std::remove(uninterested.begin(),
      // uninterested.end(), peer_port), uninterested.end());
      cout << "recevied uninterested by" << peer_port << "length prefix"
           << length_prefix << endl;
      std::unique_lock<std::shared_mutex> mtxforinterested7(mtxforallinterested);
      // std::unique_lock<std::shared_mutex>
      // mtxforuninterested7(mtxforuninterested);
      uninterested.push_back(peer_port);
      interested.erase(
          std::remove(interested.begin(), interested.end(), peer_port),
          interested.end());
      delete[] temp_buffer;
    }
    else if (message_id == 4) // have
    {
      cout << "recevied have by" << peer_port << "length prefix" << length_prefix << endl;
      map<uint32_t, uint32_t> connected_fds_map_con_copy_have;
      map<uint32_t, vector<uint32_t>> piece_info_have;
      vector<uint32_t> interested_have;
      vector<uint32_t> for_rarest_have;
      map<uint32_t, uint32_t> peer_speed_have;
      vector<uint32_t> pieces_ihave_ooyaehhh_have;
      vector<uint32_t> I_interested_have;
      map<uint32_t, uint32_t> piece_info_vec_have;
      {
        std::shared_lock<std::shared_mutex> txforconnected_fds_map_con11(mtxforconnected_fds_map_con);
        std::unique_lock<std::shared_mutex> mtxforpiece_info987(mtxforallpiece_info);
        std::unique_lock<std::shared_mutex> mtxforI_interested9(mtxforallinterested);
        std::shared_lock<std::shared_mutex> mtxforxih(mtxforpieces_ihave_ooyaehhh);
        std::shared_lock<std::shared_mutex> mtxforpeer_speed15(mtxforpeer_speed);
        std::unique_lock<std::shared_mutex> mtxforfor_rarest13(mtxforfor_raresandgetrightidt);
        connected_fds_map_con_copy_have = connected_fds_map_con;
        piece_info_have = piece_info;
        interested_have = interested;
        for_rarest_have = for_rarest;
        peer_speed_have = peer_speed;
        pieces_ihave_ooyaehhh_have = pieces_ihave_ooyaehhh;
        I_interested_have = I_interested;
        // piece_info_vec_have = piece_info_vec;
      }
      uint8_t message_id = 2;
      uint32_t piece_id_hav;
      memcpy(&piece_id_hav, temp_buffer + 5, 4);
      piece_id_hav = ntohl(piece_id_hav);
      piece_info_have[piece_id_hav].push_back(peer_port);     /// passing the port to the vector iwth pice id being  the     /// key
      piece_info_reverse[peer_port].push_back(piece_id_hav);  // reversing the above to acces with port being key and // vector of the pieces being th vector
      if (piece_info_reverse[peer_port].size() == num_pieces) // agr sre pieces agye to fhir seeder
      {
        seeders.push_back(peer_port);
      }
      uint32_t i = 0;
      {
        // std::unique_lock<std::shared_mutex>
        // mtxforfor_rarest13(mtxforfor_raresandgetrightidt);

        for (auto &meow : piece_info_have)
        {
          cout << meow.second.size() << "meow.second.size()" << endl;
          for_rarest_have[i] = (meow.second.size());         //=getting the size of the vactor to find
                                                             // the rarest piece
          piece_info_vec[meow.second.size()] = (meow.first); // size being the key and the piece being the value
          i++;
        }
        getrightid = 0;
        cout << "rarest" << for_rarest_have.size() << endl;            //  taki agr mrpe rarest ho
        qs.quicksort(for_rarest_have, for_rarest_have.size(), no_use); // sorting it
                                                                       // piece_info[for_rarest[0]];
        vector<uint32_t> tempport;                                     // has all the peersthat have the rarest
                                                                       // std::unique_lock<std::shared_mutex>
                                                                       // getrightid12(mtxforgetrightid);
        for (auto &rigthpieceiditr : pieces_ihave_ooyaehhh_have)
        {
          if (rigthpieceiditr == piece_info_vec[for_rarest_have[getrightid]])
          {
            getrightid++;
            continue;
          }
          else
            break;
        }
        for (auto &for_interested : piece_info_have[piece_info_vec[for_rarest_have[getrightid]]]) // piece_info_vec[for_rarest[0]]
                                                                                                  // gives the piece id  :
                                                                                                  // piece_info[piece_info_ve
        {
          temport.push_back(
              for_interested); // pasisng the value tempport contaisn the port
        }
      }

      for (auto &sync : temport)
      {
        tempspeed.push_back(
            peer_speed[sync]); // paiing the port to get the spped linked to
                               // that port mrko yha ma bhi thread lock krne
                               // pdage for the peer speed
      }

      cout << "tempspeed" << tempspeed.size() << endl;
      qs.quicksort(tempspeed, tempspeed.size(),
                   temport); // sorted the ports on basis of speed
      for (auto &finalsync : temport)
      {
        I_interested_have.push_back(finalsync); // abb I_interested ma sirf ma ki
                                                // chuut ski bc solve hogye
        forpublic.sendfxn_normal(message_id, connected_fds_map_con_copy_have[finalsync]);
      } // bc ak cheez to reh gye agr mrpe hua piece to fhir akk or check lgana
        // pdage
      {
        std::shared_lock<std::shared_mutex> txforconnected_fds_map_con11(mtxforconnected_fds_map_con);
        std::unique_lock<std::shared_mutex> mtxforpiece_info987(mtxforallpiece_info);
        std::unique_lock<std::shared_mutex> mtxforI_interested9(mtxforallinterested);
        std::shared_lock<std::shared_mutex> mtxforpeer_speed15(mtxforpeer_speed);
        std::unique_lock<std::shared_mutex> mtxforfor_rarest13(mtxforfor_raresandgetrightidt);
        connected_fds_map_con = connected_fds_map_con_copy_have;
        piece_info = piece_info_have;
        interested_have = interested_have;
        for_rarest = for_rarest_have;
        peer_speed = peer_speed_have;
        pieces_ihave_ooyaehhh_have = pieces_ihave_ooyaehhh;
        I_interested = I_interested_have;
      }
      delete[] temp_buffer;
    }
    else if (message_id == 6) // req
    {

      uint32_t offset;
      uint32_t length;
      uint32_t piece_id;
      memcpy(&piece_id, temp_buffer + 5, 4);
      memcpy(&offset, temp_buffer + 9, 4);
      memcpy(&length, temp_buffer + 13, 4);
      piece_id = ntohl(piece_id);
      offset = ntohl(offset);
      length = ntohl(length);
      uint8_t message_id = 7;
      {
        cout << "recevied req by" << peer_port << "length prefix" << length_prefix << endl;

        thread([&]()

               { cout<<" startign threda processing for sendreqfxn"<<endl;
                 std::shared_lock<std::shared_mutex> forpiecesihave17(mtxforpieces_ihave_ooyaehhh);
                std::shared_lock<std::shared_mutex> forunchocked_by_me16(mtxforallchoked);
                cout << "accquired lock"<< endl;
                for (auto &uncheck : unchocked_by_me)
                 {
                    if (uncheck == peer_port)
                  {
                   // cout<<"aquired lock for allchoked and pieces_ihave_ooyaehhh at sendreqfxn"<<endl;
                   
                    for (auto &piecesend : pieces_ihave_ooyaehhh) 
                    {
                      if (piece_id == piecesend) 
                      {
                        cout<<"calling sendfxn_piece from sendreqfxn"<<endl;
                        connectionhandler().sendfxn_piece(length_prefix, message_id, piece_id,offset, length, connectedfd);
                        cout << "##########blocksend done confirmed 100#########" << endl;
                        break; 
                } 
             }
              }
                 }
                 cout << "unlock for allchoked and pieces_ihave_ooyaehhh at sendreqfxn" << endl; })
            .detach();
        delete[] temp_buffer;
      }
    }
    else if (message_id == 7)
    {
      cout << "recevied piece by" << peer_port << endl;
      thread([&]()
             {
        if (!ogseeder)
        {
          if (!IamSeeder)
          {
            if (length_prefix < 9 || length_prefix > max_message_size)
            {
              delete[] temp_buffer;
              return;
            }
            uint32_t length;
            uint32_t index;
            uint32_t offset; // offset inside the piece
            // - 9 for message id index offset
            memcpy(&index, temp_buffer + 5, 4);
            memcpy(&offset, temp_buffer + 9, 4);
            memcpy(&length, temp_buffer + 13, 4);
            length = ntohl(length);
            uint8_t *data = new uint8_t[length];
            memcpy(&data[0], temp_buffer + 17, length);
            index = ntohl(index);
            offset = ntohl(offset);
            cout << "index" << index << "offset" << offset << endl;
            fstream inputfile;
            inputfile.open("/home/kartik/Downloads/" + to_string(my_port) + "/" + to_string(index) + ".temp", ios::in | ios::out | ios::app | ios::binary);
            if (inputfile.is_open())
            {
              inputfile.seekp(offset);
              if (inputfile.fail())
              {
                std::cerr << "Failed to seek in file for piece " << index << endl;
                delete[] data;
                delete[] temp_buffer;
                return;
              }
              inputfile.write(reinterpret_cast<const char *>(data), length);
              if (inputfile.fail())
              {
                std::cerr << "Failed to write data to file for piece " << index << endl;
              }
            }
            else
            {
              std::cerr << "Failed to open file for piece " << index << endl;
              delete[] data;
              delete[] temp_buffer;
              return;
            }
            std::unique_lock<std::shared_mutex> forpiecesihave107(mtxforpieces_ihave_ooyaehhh);
            if (index == num_pieces - 1)
            {
              if (piece_block_map[index].size() == no_of_block_in_unique_piece)
              {
                cout << "got piece  " << index << "   yeahh" << endl;
                if (forhash.extension_giver_cum_piecehash(index))
                {
                  cout << " hash matched yeahh of" << index << endl;
                  pieces_ihave_ooyaehhh.push_back(index);
                }
                else
                {
                  cout << "hash matcheing failed of" << index << endl;
                  string forclean = "/home/kartik/Downloads/" + to_string(my_port) +
                                    "/" + to_string(index) + ".temp";
                  std::remove(forclean.c_str());
                  piece_block_map.erase(index);
                }
              }
              else
              {
                // std::unique_lock<std::shared_mutex> forpiecesihave107(mtxforpieces_ihave_ooyaehhh);
                if (length_prefix == unique_block_len_of_unique_pi + 9 && offset == block_len_global * (no_of_block_in_unique_piece - 1))
                {
                  piece_block_map[index].push_back(no_of_block_in_unique_piece);
                  quick_sort().quicksort(piece_block_map[index], piece_block_map[index].size(), no_use);
                }
                else
                {
                  piece_block_map[index].push_back((offset + block_len_global) / block_len_global);
                  quick_sort().quicksort(piece_block_map[index], piece_block_map[index].size(), no_use);
                }
              }
            }
            else
            {
              // std::unique_lock<std::shared_mutex> forpiecesihave107(mtxforpieces_ihave_ooyaehhh);
              uint32_t counter = 0;

              if (piece_block_map[index].size() == no_of_block_in_normal_piece)
              {
                // cout << "got piece  " << index << "   yeahh" << endl;
                if (forhash.extension_giver_cum_piecehash(index))
                {
                  cout << " hash matched yeahh of" << index << endl;
                  pieces_ihave_ooyaehhh.push_back(index);
                }
                else
                {
                  cout << "hash matcheing failed of" << index << endl;
                  string forclean = "/home/kartik/Downloads/" + to_string(my_port) +
                                    "/" + to_string(index) + ".temp";
                  std::remove(forclean.c_str());
                  piece_block_map.erase(index);
                }
              }
              else
              {
                piece_block_map[index].push_back((offset + block_len_global) / block_len_global);
                quick_sort().quicksort(piece_block_map[index], piece_block_map[index].size(), no_use);
              }
            }
            for (auto &havemsg : connected_fds_map_con)
            {
              uint8_t send_state = snd_have;
              uint32_t connectedfd = havemsg.second;
              uint8_t message_id = 4;
              uint32_t length_prefix = 1;
              forpublic.sendfxn_normal(message_id, connectedfd);
            }
            inputfile.close();
            delete[] data;
            delete[] temp_buffer;
          }
        } })
          .detach();
    }
    else if (message_id == 8)
    { // cancel
      delete[] temp_buffer;
    }
    else if (message_id == 9) // custom handsahke
    {
      if (memcmp(temp_buffer + 1, file_hash, 20) != 0)
      {
        shutdown(connectedfd, SHUT_RDWR); // solve the removal of closed fd in future not done
                                          // currently peer_info.erase(connectedfd); // solved
                                          // the above problrm
      }
      delete[] temp_buffer;
    }
  };
  void first_five() // abhi to seedeer vlefd use krenga kyunkifile_info first
                    // five are from seeder only
  {
    // std::shared_lock<std::shared_mutex>
    // mtxforpieces_ihave_ooyaehhh18(mtxforpieces_ihave_ooyaehhh);

    uint32_t piece_id;                  // jutified
    uint32_t length = block_len_global; // bc ma sas kr ste tha ki ma first five
                                        // ma unique piece download kr lte
    uint32_t block_num =
        piece_length_global / block_len_global; // bc kya kra hua ma
    uint32_t offset;
    uint32_t connectedfd;
    uint32_t randseed;
    connectionhandler forfirst5;
    uint32_t length_prefix = 1;
    uint8_t message_id1 = 2;
    uint32_t tempid = UINT32_MAX;
    cout << "first five called" << endl;
    //  map<uint32_t, uint32_t> connected_fds_map_con_copy_have;
    //  map<uint32_t, vector<uint32_t>> piece_info_have;
    /// vector<uint32_t> interested_have;
    // vector<uint32_t> for_rarest_have;
    // map<uint32_t, uint32_t> peer_speed_have;
    // vector<uint32_t> pieces_ihave_ooyaehhh_have;
    // vector<uint32_t> I_interested_have;
    // {
    //    std::shared_lock<std::shared_mutex> txforconnected_fds_map_con11(mtxforconnected_fds_map_con);
    //  std::unique_lock<std::shared_mutex> mtxforpiece_info987(mtxforallpiece_info);
    // std::unique_lock<std::shared_mutex> mtxforI_interested9(mtxforallinterested);
    // s//td::unique_lock<std::shared_mutex> mtxforfor_rarest13(mtxforfor_raresandgetrightidt);
    // std::shared_lock<std::shared_mutex> mtxforpeer_speed15(mtxforpeer_speed);
    //   std::shared_lock<std::shared_mutex> mtxforxih(mtxforpieces_ihave_ooyaehhh);
    //   std::shared_lock<std::shared_mutex> jhgjh(mtxforallchoked);
    //   connected_fds_map_con_copy_have = connected_fds_map_con;
    // piece_info_have = piece_info;
    // interested_have = interested;
    // for_rarest_have = for_rarest;
    // peer_speed_have = peer_speed;
    //    pieces_ihave_ooyaehhh_have = pieces_ihave_ooyaehhh;
    // I_interested_have = I_interested;
    bool firsttime = true;
    //  }
    while (first_5)
    {
      std::shared_lock<std::shared_mutex> mtxforconnected_fds_map_con19(mtxforconnected_fds_map_con);
      std::shared_lock<std::shared_mutex> mtxforpieces_ihave_ooyaehhh18(mtxforpieces_ihave_ooyaehhh);
      std::shared_lock<std::shared_mutex> jhgjh(mtxforallchoked);
      // piece_id = rand() % num_pieces;
      cout << "firts five loop" << endl;
      if (!firsttime)
      {
        if (piece_id == num_pieces - 1)
        {
          if (piece_block_map[piece_id].size() == no_of_block_in_unique_piece)
          {
            tempid = UINT32_MAX;
            piece_block_map.erase(piece_id);
          }
        }
        else if (tempid < num_pieces - 1)
        {
          if (piece_block_map[piece_id].size() == no_of_block_in_normal_piece)
          {
            piece_block_map.erase(piece_id);
            tempid = UINT32_MAX;
          }
        }
      };
      firsttime = false;
      if (tempid == UINT32_MAX || firsttime)
      {
        if (!pieces_ihave_ooyaehhh.empty())
        {
          uint8_t ctr_lcl = 0;
          bool piecent_found = true;
          piece_id = rand() % num_pieces;
          while (piecent_found && ctr_lcl < 100)
          {

            piece_id = rand() % (num_pieces);
            piecent_found = false;
            for (auto &piece : pieces_ihave_ooyaehhh)
            {
              if (piece_id == piece)
              {
                piecent_found = true;
                break;
              }
            }
            ctr_lcl++;
          }
          if (pieces_ihave_ooyaehhh.size() == num_pieces)
          {
            return;
          }
        }
        else
        {
          piece_id = rand() % (num_pieces);
        }
        bool hlo = false;
        randseed = rand() % (seeders.size());
        cout << "(seeders.size())" << (seeders.size()) << " seeder" << *seeders.data() << endl;
        for (auto &abcd : seeders)
        {
          for (auto &abc : unchocked_by)
          {
            if (abcd == abc)
            {
              connectedfd = connected_fds_map_con[abcd];
              cout << "abcd" << abcd << " connectedfd " << connected_fds_map_con[abcd] << endl;
              hlo = true;
              break;
            }
          }
          if (hlo)
          {
            break;
          }
        }
        if (piece_id == unique_piece)
        {
          block_num = no_of_block_in_unique_piece;
        }
        else
        {
          block_num = no_of_block_in_normal_piece;
        }
        connectedfd = connected_fds_map_con[8080];
        cout << "sending req for first five" << connectedfd << "   " << connected_fds_map_con[8080] << " " << piece_id << "  " << block_num << " " << unique_block_len_of_unique_pi << " " << no_of_block_in_normal_piece << endl;
        // forfirst5.sendfxn_normal(message_id1, randseed);
        for (uint32_t i = 0; i < block_num; i++)
        {
          cout << "workeed times  meow" << i << endl;
          if (i == unique_block_len_of_unique_pi - 1 && piece_id == num_pieces - 1)
          {
            length = unique_block_len_of_unique_pi;
          }
          else if (piece_id == num_pieces - 1 && i == no_of_block_in_normal_piece - 1)
          {
            length = unique_block_len_of_normal;
          }
          else
          {
            length = block_len_global;
          }
          // if (i == 1)
          //{
          //  cout << "breaking to test" << endl;
          //  return;
          // }
          offset = block_len_global * i;
          cout << "req hloo piece_id " << piece_id << " offset" << offset << " length " << length << " i " << i << "blocknum" << block_num << endl;
          forfirst5.sendfxn_req(piece_id, offset, length, connectedfd);
          cout << "&&&&&&&&&&sendfxn_req completeed&&&&&&&&&" << endl;
          sleep(5);
        }
      }
      else
      {
        if (piece_id == num_pieces - 1)
        {
          for (auto &agay : piece_block_map[piece_id])
          {
            uint32_t i = 0;
            if (i == agay)
            {
              continue;
            }
            else
            {
              if (i == no_of_block_in_unique_piece - 1)
              {
                offset = block_len_global * (i);
                length = unique_block_len_of_unique_pi;
              }
              else
              {
                offset = i * block_len_global;
                length = block_len_global;
              }
              forfirst5.sendfxn_req(piece_id, offset, length, connectedfd);
            }
          }
        }
        else
        {
          for (auto &agay : piece_block_map[piece_id])
          {
            uint32_t i = 0;
            if (i == agay)
            {
              continue;
            }
            else
            {
              if (i == no_of_block_in_normal_piece - 1)
              {
                length = unique_block_len_of_normal;
              }
              else
              {
                length = block_len_global;
              }
              offset = i * block_len_global;
              // length = block_len_global;
              forfirst5.sendfxn_req(piece_id, offset, length, connectedfd);
            }
          }
        }
      }
      if (pieces_ihave_ooyaehhh.size() == 5)
      {
        cout << "got first five pieces" << endl;
        std::unique_lock<std::shared_mutex> mtxforuhu(mtxforfirstfive);
        first_5 = false;
        other = true;
      }
      tempid = piece_id;
      sleep(5);
    }
  }
  void other_pieces() // dont judge plzz
  {
    if (other)
    {
      cout << "otehr pieces calles" << endl;
      sleep(2);
      connectionhandler forother;
      uint32_t piece_id;
      uint32_t length = block_len_global;
      uint32_t block_num;
      uint32_t rem;
      uint32_t counter = 0;
      uint32_t unique_block_len_of_unique_pi;
      uint32_t realremainder;
      uint32_t length_prefix = 1;
      uint8_t message_id1 = 2;
      // uint32_t temp_pieceid = num_pieces + 1; // taki ma same piece na
      // downlaod kru kyunki same rarest o skt ha
      bool namkhtm; // neeche dakhne pdage bc ki jbb pahle write kre that tbb
                    // theek thabc hrr jgh shah1 ghusa dunga mkc
      uint32_t connectedfd;
      uint32_t offset;
      bool firsttime = true;
      uint32_t tempid = UINT32_MAX;

      cout << "openeing loop downloaded yest" << endl;
      map<uint32_t, uint32_t> connected_fds_map_con_copy_have;
      map<uint32_t, vector<uint32_t>> piece_info_have;
      /// vector<uint32_t> interested_have;
      vector<uint32_t> for_rarest_have;
      // map<uint32_t, uint32_t> peer_speed_have;
      vector<uint32_t> pieces_ihave_ooyaehhh_have;
      vector<uint32_t> I_interested_have;
      vector<uint32_t> unchocked_by_have;
      map<uint32_t, uint32_t> piece_info_vec_have;
      map<uint32_t, vector<uint32_t>> piece_block_map_other;
      {
        std::shared_lock<std::shared_mutex> txforconnected_fds_map_con11(mtxforconnected_fds_map_con);
        std::unique_lock<std::shared_mutex> mtxforpiece_info987(mtxforallpiece_info);
        std::unique_lock<std::shared_mutex> mtxforI_interested9(mtxforallinterested);
        // s//td::unique_lock<std::shared_mutex> mtxforfor_rarest13(mtxforfor_raresandgetrightidt);
        // std::shared_lock<std::shared_mutex> mtxforpeer_speed15(mtxforpeer_speed);
        //  std::shared_lock<std::shared_mutex> mtxforxih(mtxforpieces_ihave_ooyaehhh);
        // std::shared_lock<std::shared_mutex> jhgjh(mtxforallchoked);
        connected_fds_map_con_copy_have = connected_fds_map_con;
        // piece_info_have = piece_info;
        // interested_have = interested;
        for_rarest_have = for_rarest;
        // peer_speed_have = peer_speed;
        pieces_ihave_ooyaehhh_have = pieces_ihave_ooyaehhh;
        unchocked_by_have = unchocked_by;
        piece_info_vec_have = piece_info_vec;
        piece_block_map_other = piece_block_map;
        // I_interested_have = I_interested;
      }
      while (downloaded_yet)
      {
        sleep(2);
        connectedfd = UINT32_MAX;
        // std::shared_lock<std::shared_mutex> mtxforconnected_fds_map_con22(mtxforconnected_fds_map_con);
        // std::shared_lock<std::shared_mutex> mtxforpiece_info23(mtxforallpiece_info);
        // std::shared_lock<std::shared_mutex> mtxforI_interested21(mtxforallinterested);
        {
          //  std::shared_lock<std::shared_mutex> mtxforgetrightid1(mtxforfor_raresandgetrightidt);
          // std::shared_lock<std::shared_mutex>
          // mtxforfor_rarest24(mtxforfor_rarest);
          // std::shared_lock<std::shared_mutex>
          // mtxforpiece_info_vec25(mtxforpiece_info_vec);
          while (for_rarest_have.empty())
          {
            cout << "for rarest other pieces" << endl;
            sleep(1);
          }

          /*  piece_id = piece_info_vec_have[for_rarest_have[getrightid]];
            if (getrightid == -1)
            {
              throw ::runtime_error("could not get rarest piece");
            }
            // piece_id = piece_info_vec[for_rarest[getrightid]];
            if (piece_id == num_pieces)
            {
              if (unique_piece % block_len_global != 0)

              {
                realremainder = unique_piece % block_len_global;
                block_num = (unique_piece - realremainder) / block_len_global;
                block_num++;
                unique_block_len_of_unique_pi = realremainder;
              }
              else
              {
                block_num = unique_piece / block_len_global;
                length = block_len_global;
              }
            }
            if (piece_id != num_pieces)
            {
              length = block_len_global; // length block ki hoga na naki piece ki
                                         // chutiyaa
              block_num = piece_length_global / block_len_global;
            }
          }
          {
            std::shared_lock<std::shared_mutex> nmnmj(mtxforallchoked);
            for (auto &toocheck : I_interested_have)
            {
              for (auto &tocheck : unchocked_by_have)
              {
                if (toocheck == tocheck)
                {
                  connectedfd = connected_fds_map_con_copy_have[toocheck];
                  break;
                }
              }
            }
            if (connectedfd == UINT32_MAX)
            {
              connectedfd = connected_fds_map_con_copy_have[*(seeders.data() +
                                                              rand() % (seeders.size()))];
            }
            if (connectedfd != UINT32_MAX)
            {
              {
                // std::shared_lock<std::shared_mutex> sr4(mtxforrecv);
                cout << "stepped into mtxsndrecv sr4" << endl;
                ch2.sendfxn_normal(message_id1, connectedfd);
                sleep(4);
                for (uint32_t i{}; i < block_num; i++)
                {
                  if (piece_id == num_pieces &&
                      unique_piece / block_len_global != 0)
                  {
                    if (block_num == i + 1)
                    {
                      length = unique_block_len_of_unique_pi;
                    }
                  }
                  length = block_len_global;
                  offset = length * i;
                  cout << "req2" << piece_id << offset << length << endl;
                  ch2.sendfxn_req(piece_id, offset, length, connectedfd); // send message to req the piece
                }
                counter++;
                if (counter == (num_pieces - 5))
                {
                  downloaded_yet = false;
                }
              }
            }
          }
        }

          for (auto &toocheck : I_interested_have)
          {
            for (auto &tocheck : unchocked_by_have)
            {
              if (toocheck == tocheck)
              {
                connectedfd = connected_fds_map_con_copy_have[toocheck];
                break;
              }
            }
          }
          if (connectedfd == UINT32_MAX)
          {
            connectedfd = connected_fds_map_con_copy_have[*(seeders.data() +
                                                            rand() % (seeders.size()))];
          }
  */
          piece_id = piece_info_vec_have[for_rarest_have[getrightid]];
          if (!firsttime)
          {
            if (piece_id == num_pieces - 1)
            {
              if (piece_block_map_other[piece_id].size() == no_of_block_in_unique_piece)
              {
                tempid = UINT32_MAX;
                piece_block_map_other.erase(piece_id);
              }
            }
            else if (tempid < num_pieces - 1)
            {
              if (piece_block_map_other[piece_id].size() == no_of_block_in_normal_piece)
              {
                tempid = UINT32_MAX;
                piece_block_map_other.erase(piece_id);
              }
            }
          };
          connectedfd = UINT32_MAX;
          piece_id = piece_info_vec_have[for_rarest_have[getrightid]];
          firsttime = false;
          if (tempid == UINT32_MAX || firsttime)
          {
            if (!pieces_ihave_ooyaehhh.empty())
            {
              for (auto &toocheck : I_interested_have)
              {
                for (auto &tocheck : unchocked_by_have)
                {
                  if (toocheck == tocheck)
                  {
                    connectedfd = connected_fds_map_con_copy_have[toocheck];
                    break;
                  }
                }
              }
              if (connectedfd == UINT32_MAX)
              {
                for (auto &tocheck : unchocked_by_have)
                {
                  if (*(seeders.data() + rand() % (seeders.size())) == tocheck)
                  {
                    connectedfd = connected_fds_map_con_copy_have[tocheck];
                    break;
                  }
                }
              }
            }
            if (piece_id == unique_piece)
            {
              block_num == unique_block_len_of_unique_pi;
            }
            else
            {
              block_num = no_of_block_in_normal_piece;
            }
            cout << "sending req for first five" << endl;
            // forother.sendfxn_normal(message_id1, connectedfd);
            for (uint32_t i = 0; i < block_num; i++)
            {
              // piece_block_map[piece_id];
              if (i == unique_block_len_of_unique_pi - 1 && piece_id == num_pieces - 1)
              {
                length = unique_block_len_of_unique_pi;
              }
              else if (piece_id == num_pieces - 1 && i == no_of_block_in_normal_piece - 1)
              {
                length = unique_block_len_of_normal;
              }
              else
              {
                length = block_len_global;
              }
              offset = block_len_global * i;
              cout << "req  " << piece_id << " " << offset << " " << length << endl;
              forother.sendfxn_req(piece_id, offset, length, connectedfd);
              sleep(3);
            }
          }
          else
          {
            if (piece_id == num_pieces - 1)
            {
              for (auto &agay : piece_block_map[piece_id])
              {
                uint32_t i = 0;
                if (i == agay)
                {
                  continue;
                }
                else
                {
                  if (i == no_of_block_in_unique_piece - 1)
                  {
                    offset = block_len_global * (i);
                    length = unique_block_len_of_unique_pi;
                  }
                  else
                  {
                    offset = i * block_len_global;
                    length = block_len_global;
                  }
                  forother.sendfxn_req(piece_id, offset, length, connectedfd);
                }
              }
            }
            else
            {
              for (auto &agay : piece_block_map[piece_id])
              {
                uint32_t i = 0;
                if (i == agay)
                {
                  continue;
                }
                else
                {
                  if (i == no_of_block_in_normal_piece - 1)
                  {
                    length = unique_block_len_of_normal;
                  }
                  else
                  {
                    length = block_len_global;
                  }
                  offset = i * block_len_global;
                  // length = block_len_global;
                  forother.sendfxn_req(piece_id, offset, length, connectedfd);
                }
              }
            }
          }
          if (pieces_ihave_ooyaehhh.size() == num_pieces)
          {
            cout << "got first five pieces" << endl;
            // first_5 = false;
            other = false;
          }
          tempid = piece_id;
          sleep(5);
        }

        {
          std::shared_lock<std::shared_mutex> txforconnected_fds_map_con11(mtxforconnected_fds_map_con);
          std::unique_lock<std::shared_mutex> mtxforpiece_info987(mtxforallpiece_info);
          std::unique_lock<std::shared_mutex> mtxforI_interested9(mtxforallinterested);
          // s//td::unique_lock<std::shared_mutex> mtxforfor_rarest13(mtxforfor_raresandgetrightidt);
          // std::shared_lock<std::shared_mutex> mtxforpeer_speed15(mtxforpeer_speed);
          //  std::shared_lock<std::shared_mutex> mtxforxih(mtxforpieces_ihave_ooyaehhh);
          // std::shared_lock<std::shared_mutex> jhgjh(mtxforallchoked);
          connected_fds_map_con = connected_fds_map_con_copy_have;
          piece_info = piece_info_have;
          // interested_have = interested;
          for_rarest = for_rarest_have;
          // peer_speed_have = peer_speed;
          pieces_ihave_ooyaehhh = pieces_ihave_ooyaehhh_have;
          piece_info_vec = piece_info_vec_have;
          piece_block_map = piece_block_map_other;
          // I_interested_have = I_interested;
        }
      }
    }
  }
};
class Peer
    : public my_jason // handles peers oo yeahhh baw baw meow meowww hurr hurr
{
public:
  const char *peer_ip;
  uint32_t peer_port;
  // conctructor vle map
  int peer_fd;
  struct sockaddr_in peer_address;
  updater_cum_download update;
  vector<int> temp;
  bool pipe_laid;
  int peer_id; // fds jo peer using krnege to connect toothers;
  int state;
  int con;
  uint32_t client_port;
  uint32_t connectedfd;
  bool sothatconcanuserev;
  // int socket;
  connectionhandler foralluse;
  Peer(const uint32_t &peer_port, const char *peer_ip, const uint8_t &peer_id,
       const uint8_t &state, int &my_socket)
  {
    // this->connectedfd = connectedfd;
    this->peer_address = peer_address;
    this->peer_fd = socket(AF_INET, SOCK_STREAM, 0);
    this->peer_port = peer_port;
    this->peer_ip = peer_ip;
    this->peer_address.sin_family = AF_INET;
    this->peer_address.sin_port = htons(peer_port);
    this->state = state;
    inet_pton(AF_INET, peer_ip, &this->peer_address.sin_addr.s_addr);
    this->peer_fd = peer_fd;
    this->sothatconcanuserev = sothatconcanuserev;
    this->client_port = client_port;
    if (this->peer_fd == -1)
    {
      std::cerr << "socket creation on port failed" << peer_port << strerror(errno) << endl;
      return;
    }
    else
    {
      cout << "socket creation on port completed" << peer_port << endl;
    }
    socklen_t size = sizeof(sockaddr_in);

    if (my_port < peer_port)
    { // will use sothatconcanuserev =true;
      if (peer_fd != -1)
      {
        cout << " closing peer " << my_port << peer_port << endl;
        shutdown(peer_fd, SHUT_RDWR);
        this->sothatconcanuserev = true;
      }
    }
    else
    { // will use sothatconcanuserev =false;
      if (peer_fd != -1)
      {
        /*  string ip ="127.0.0."+to_string(peer_id);
             inet_pton(AF_INET, ip.c_str(), &server_address.sin_addr);
             socklen_t size = sizeof(server_address);
            int bn = bind(this->peer_fd, (sockaddr *)&server_address, size);
       if (bn == 0) {
         cout << "bind succesfull" << peer_port << "   ip    " << peer_ip << endl;
         else
      {
        cout << "bind was not successfull" << endl;
      }*/
        size = sizeof(peer_address);
        int con = connect(this->peer_fd, (sockaddr *)&peer_address, size);
        if (con == 0)
        {

          std::unique_lock<std::shared_mutex> mtxforconnected_fds_map_con17(mtxforconnected_fds_map_con);
          cout << "pushing connected fd" << peer_fd << "at port " << peer_port << endl;
          connected_fds_map_con[peer_port] = peer_fd;
          connectedfd = peer_fd;
          ports.push_back(peer_port);
          this->sothatconcanuserev = false;
          client_port = peer_port; // setting the client port to the
          cout << "connected at port " << peer_port << client_port << " ip " << peer_ip << endl;
        }
        else
        {
          std::cerr << "connect fxn failed at port and ip" << this->peer_port << this->peer_ip << "  " << strerror(errno) << endl;
          this->sothatconcanuserev = false;
          shutdown(peer_fd, SHUT_RDWR);
          throw std::runtime_error("connect fxn failed at port and ip");
        }
      }
    }
    cout << "starting thread recv" << endl;
    bool mkl = false;
    std::thread([&, this]
                { 

      if (sothatconcanuserev) {
        struct sockaddr_in incoming_address{};
        socklen_t peeraddr_size = sizeof(incoming_address);
         cout<<"waiting for recv"<<endl;
        int acc = accept(my_socket, (sockaddr *)&incoming_address, &peeraddr_size);
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &incoming_address.sin_addr, ip_str, sizeof(ip_str));
        uint16_t client_port_16 = ntohs(incoming_address.sin_port);
        client_port = uint32_t(client_port_16);
        if (acc != -1) {
          cout << "accepted at port " << client_port << " ip " << ip_str
               << endl;
          ports.push_back(client_port);
        } else {
          cout << "accept failed" << endl;
          mkl = true;
        }
        if (!mkl) {
          // incoming_address.sin_port = ntohl(incoming_address.sin_port);
          /* if (my_port > client_port)
           {
             cout << "closing at port" << client_port << endl;
             close(acc);
           }
             */
          // else
          // {
          connectedfd = (uint32_t)acc;
          std::unique_lock<std::shared_mutex> mtxforconnected_fds_map_con30(mtxforconnected_fds_map_con);
          connected_fds_map_con[client_port] = connectedfd;
          cout << "pushing connected fd" << connectedfd << "at port "<< client_port<<endl;
          // }}
        }
      }
      while (true) {
        cout << "******starting recv thread******" << endl;
        sleep(3);
        thread([&, this]()
               {
               // std::unique_lock<std::shared_mutex> sr1(mtxforsendrecv);
          vector<uint8_t> local_buffer;
          uint32_t length_prefix = 0;
          uint8_t length_prefix_buff[4];
          // length_prefix=ntohl(length_prefix);
          uint32_t rec_finished = 0;
          int error_check = 0;
          uint32_t rec_current = 0;
          //cout << "starting loop recv" << endl;
          {
           // cout << "waiting for thread at line 928" << endl;
           // std::shared_lock<std::shared_mutex> sr5(mtxforrecv);
           //  cout<<"stepped into mtxsndrecv sr5"<< endl;
           // cout << "wait ended at line 930" << endl;
            int socket_error = 0;
            socklen_t len = sizeof(socket_error);
                 int retval = getsockopt(connectedfd, SOL_SOCKET, SO_ERROR, &socket_error, &len);
              if (retval == 0 || socket_error == 0) {
            cout << "Socket error: " << strerror(socket_error) << endl;
         // return;
          }
         // cout << "while for recv started" <<" at_fd " << connectedfd << endl;
         
          while (rec_finished < 4)
          {
            int error = 0;
            socklen_t len = sizeof(error);
            if (getsockopt(connectedfd, SOL_SOCKET, SO_ERROR, &error, &len) != 0 || error != 0)
            {
              printf("Socket is dead, exiting receive loop\n");
              break;
            }
            error_check = recv(connectedfd, length_prefix_buff + rec_finished, 4 - rec_finished, 0); // first i am reading length prefix
            cout << "buffer  " << *(length_prefix_buff + rec_finished) << endl;
            
            if (error_check == 0)
            {
              cout << "Connection closed by peer" << endl;
              break;
            }
            else if (error_check == -1)
            {
              cout << "recv() error: " << strerror(errno) <<" fd is  "<<connectedfd <<endl;
              break;
            }
            rec_current = (uint32_t)error_check;
            rec_finished = rec_current + rec_finished;
            cout << " rec_current" << rec_current << " " << rec_finished << endl;
            
          }
          //cout << "Bytes of " << num << " are: ";
          for (int i = 0; i < 4; i++)
          {
            cout << +length_prefix_buff[i] << " "; // +arr[i] promotes uint8_t to int for printing
          }
          cout << endl;
          memcpy(&length_prefix, length_prefix_buff, 4);
         // cout << "lenght prefix at just below recv lop" << length_prefix << endl;

          // if (length_prefix >9000)
          // {
          length_prefix = ntohl(length_prefix);
          cout << "lenght prefix at just below recv lop" << length_prefix << endl;
          // }
          bool passed;
          if (error_check == -1)
          {
            cout << "error check failed at line 947" << endl;
            sleep(2);
            passed = false;
            exit(0);
          } else {
              passed = true;
            }
            if (passed) {
              if(length_prefix < 1 || length_prefix > 1000000)
              {
                cout << "length prefix is not in range 1 to 1000000" << endl;
                sleep(2);
                return;
              }
              uint8_t *info_arry = new uint8_t[length_prefix + 4];
              rec_current = 0;
              rec_finished = 0;
              error_check = 0;
              cout << "starting  loop real recv" << endl;
              auto start = high_resolution_clock::now();
              while (rec_finished < length_prefix) 
              {
                int error = 0;
                socklen_t len = sizeof(error);
                if (getsockopt(connectedfd, SOL_SOCKET, SO_ERROR, &error, &len) != 0 || error != 0)
                {
                  printf("Socket is dead, exiting receive loop\n");
                  break;
                }
                rec_current = recv(connectedfd, info_arry + 4 + rec_finished,length_prefix - rec_finished, 0);
                if (error_check == -1) 
                {
                  if(errno== EAGAIN || errno==EWOULDBLOCK)
                  {
                    continue;
                  }
                  std::cerr<<"recv_failed"<<strerror(errno)<<" fd is "<<connectedfd<<endl;
                  cout << "bteaking main recv loop" << endl;
                  sleep(2);
                  break;
                }
                else if(rec_current==0)
                {
                  std::cerr << "connection closedby perr" << endl;
                  break;
                }
                error_check = rec_current;
                rec_current = (uint32_t)error_check;
                rec_finished= rec_current+rec_finished;
                if(length_prefix==8209)
                {
                  cout << "rec_finished" << rec_finished << " length_prefix" << length_prefix << endl;
                }
              }
              auto end = high_resolution_clock::now();
              auto result = duration_cast<nanoseconds>(end - start);
              int finalspeedint = (length_prefix / result.count()) * 1000000000;
              uint32_t finalspeed = (uint32_t)finalspeedint;
              {
                std::unique_lock<std::shared_mutex> mtxforpeer_speed6(
                    mtxforpeer_speed);
                peer_speed[client_port] = finalspeed;
              }
              cout << "calling recv mesg lenght_prefix confd port  " << "  " << length_prefix << " " << connectedfd << " " << client_port <<"messageid"<<*(reinterpret_cast<int*>(info_arry+4))<< endl;
              uint32_t port = uint32_t(client_port);
              //length_prefix= ntohl(length_prefix);
            //  connectedfd = ntohl(connectedfd);
              cout << "calling recv mesg lenght_prefix confd port  " << "  " << length_prefix << " " << connectedfd << " " << client_port << "messageid" << *(reinterpret_cast<int*>(info_arry + 4)) << endl;
              update.rec_msg(length_prefix, connectedfd, info_arry, port);
            } else {
              cout << "kuch to bhayankar glt ha" << endl;
            }
          } })
            .join();
        cout << "****recv threaded ended******" << endl;
      } })
        .detach();
  }
  void filereader_cum_cumbiner() // rread all files and then combine
  {
    my_jason forcombiner;
    if (!ogseeder)
    {
      cout << "starting cum cumbiner" << endl;
      std::shared_lock<std::shared_mutex> mtxforconnected_fds_map_con30(
          mtxforconnected_fds_map_con);
      std::unique_lock<std::shared_mutex> mtxforpieces_ihave_ooyaehhh31(
          mtxforpieces_ihave_ooyaehhh);

      vector<uint8_t> filewriter;
      quick_sort quso;
      uint32_t offset_pointer = 0;
      bool done;
      uint32_t piece_length = piece_length_global; // making local copy of global
      cout << "pieces_ihave_ooyaehhh" << num_pieces << endl;
      quso.quicksort(pieces_ihave_ooyaehhh, pieces_ihave_ooyaehhh.size(), no_use); // sort krdu pieces ko taki likhne ma asani ho vse
      my_jason bkl;
      fstream dumpfile;
      int counter = 0;
      dumpfile.open("/home/kartik/Downloads/" + to_string(my_port) +
                        "dumpfile" + "/" + ".temp",
                    ios::in | ios::out | ios::app |
                        ios::binary); // dump file ma sre pieces ka data but
                                      // with out extension
      if (num_pieces == pieces_ihave_ooyaehhh.size())
      {
        IamSeeder = true;
        if (dumpfile.is_open())
        {
          for (auto &mkc : pieces_ihave_ooyaehhh)
          {
            if (mkc == num_pieces - 1)
            {
              filewriter.resize(unique_piece);
              filewriter.clear();
            }
            else
            {
              filewriter.resize(piece_length_global);
              filewriter.clear();
            }
            fstream tempfiles;
            done = false;
            tempfiles.open(
                "/home/kartik/Downloads/" + to_string(my_port) + "/" +
                    to_string(mkc) + ".temp",
                ios::in |
                    ios::binary); // as the temp file are based on piece id
            if (tempfiles.is_open())
            {
              if (mkc == num_pieces - 1 && totalfilesize % piece_length != 0)
              {
                piece_length = totalfilesize % piece_length;
              }
              tempfiles.read(reinterpret_cast<char *>(filewriter.data()),
                             piece_length);
              if (forcombiner.extension_giver_cum_piecehash(mkc))
              {
                dumpfile.write(reinterpret_cast<char *>(filewriter.data()),
                               piece_length);
                tempfiles.close();
                done = true;
              }
              else
              {
                string combine = to_string(mkc) + to_string(my_port) + ".temp";
                // const char *filenameinchar = combine.c_str;
                std::remove(combine.c_str());
              }
            }
            else
            {
              throw std::runtime_error("failed toopen " + to_string(mkc));
            }
            if (done)
            {
              pieces_ihave_ooyaehhh.erase(
                  std::remove(pieces_ihave_ooyaehhh.begin(),
                              pieces_ihave_ooyaehhh.end(), mkc));
            }
          }
        }
        else
        {
          cout << "lodel lgg gye ha ";
          dumpfile.close();
          return;
        }

        for (const auto &getexten : file_info)
        {
          for (const auto &sizenhash : getexten.second)
          {
            fstream ogfileontop;
            ogfileontop.open("/home/kartik/Downloads/" + to_string(my_port) +
                                 "ogfile" + "/" + getexten.first,
                             ios::in | ios::out | ios::app | ios::binary);
            if (ogfileontop.is_open())
            {
              filewriter.clear();
              ogfileontop.seekp(offset_pointer);
              filewriter.resize(sizenhash.first);
              dumpfile.read(
                  reinterpret_cast<char *>(filewriter.data() + offset_pointer),
                  sizenhash.first);
              if (sizenhash.second ==
                  bkl.sha1_calculator(
                      reinterpret_cast<char *>(filewriter.data()),
                      sizenhash.first))
              {
                ogfileontop.write(reinterpret_cast<char *>(filewriter.data()),
                                  sizenhash.first);
                offset_pointer = offset_pointer + sizenhash.first;
              }
              else
              {
                // abb error ka dakhne pdage bc ki jbb pahle write kre that tbb
                // theek thabc hrr jgh shah1 ghusa dunga mkc
              }
            }
            else
            {
              cout << "could not open master file";
              throw std::runtime_error("could open master file");
              dumpfile.close();
              ogfileontop.close();
            }
          }
        }
        while (!pieces_ihave_ooyaehhh.empty() && counter < 10)
        {
          filereader_cum_cumbiner(); // inn need to call this
          counter++;
        }
      }
      cout << "completed" << endl;
    }
  }
};
class chokealgo
{
  connectionhandler forchokeobj;
  connectionhandler forunchokeobj;

public:
  void choke()
  {
    map<uint32_t, uint32_t> connected_fds_map_con_copy_have;
    vector<uint32_t> unchocked_by_me_have;
    vector<uint32_t> chocked_by_me_have;
    // map<uint32_t, vector<uint32_t>> piece_info_have;
    /// vector<uint32_t> interested_have;
    // vector<uint32_t> for_rarest_have;
    // map<uint32_t, uint32_t> peer_speed_have;
    // vector<uint32_t> pieces_ihave_ooyaehhh_have;
    // vector<uint32_t> I_interested_have;
    {
      std::shared_lock<std::shared_mutex> txforconnected_fds_map_con11(mtxforconnected_fds_map_con);
      // std::unique_lock<std::shared_mutex> mtxforpiece_info987(mtxforallpiece_info);
      //  std::unique_lock<std::shared_mutex> mtxforI_interested9(mtxforallinterested);
      // s//td::unique_lock<std::shared_mutex> mtxforfor_rarest13(mtxforfor_raresandgetrightidt);
      // std::shared_lock<std::shared_mutex> mtxforpeer_speed15(mtxforpeer_speed);
      //  std::shared_lock<std::shared_mutex> mtxforxih(mtxforpieces_ihave_ooyaehhh);
      std::shared_lock<std::shared_mutex> jhgjh(mtxforallchoked);
      connected_fds_map_con_copy_have = connected_fds_map_con;
      unchocked_by_me_have = unchocked_by_me;
      chocked_by_me_have = chocked_by_me;
      //  piece_info_have = piece_info;
      // interested_have = interested;
      // for_rarest_have = for_rarest;
      // peer_speed_have = peer_speed;
      // pieces_ihave_ooyaehhh_have = pieces_ihave_ooyaehhh;
      // I_interested_have = I_interested;
    }
    cout << "stepped into mtxsndrecv sr6 " << endl;
    // std::unique_lock<std::shared_mutex> sr6(mtxforsend);
    // std::unique_lock<std::shared_mutex> mtxforconnected_fds_map_con5(mtxforconnected_fds_map_con);
    // std::unique_lock<std::shared_mutex> mtxforunchoked_by_me4(mtxforallchoked);

    uint8_t send_state = snd_choke;
    uint8_t message_id = 0;
    uint32_t length_prefix = 1;
    uint32_t connectedfd;
    int length = 2;

    for (auto &forchoke : ports) // ny this i get all teh ports macho sre port
                                 // ka liya kuch krne pdage
    {
      for (auto &forchokeint :
           unchocked_by_me_have) // agr unhoked by ni hoga tochoke krdo simple
      {
        if (forchoke != forchokeint)
        {
          connectedfd = connected_fds_map_con_copy_have[forchoke];
          cout << "choking peer with piece id" << forchoke << endl;
          chocked_by_me_have.push_back(forchoke);
          forchokeobj.sendfxn_normal(message_id, connectedfd);
          // auto start = high_resolution_clock::now();

          /* auto end = high_resolution_clock::now();
           auto result = duration_cast<nanoseconds>(end - start);
           int finalspeedint = (length / result.count()) * 1000000000000;
           uint32_t finalspeed = (uint32_t)finalspeedint;
           {
             std::unique_lock<std::shared_mutex>
         mtxforpeer_speed6(mtxforpeer_speed); peer_speed[forchoke] = finalspeed;
           }
         }*/
        }
        else
        {
          // unchocked_by_me.erase(std::remove(unchocked_by_me.begin(),
          //                                   unchocked_by_me.end(), forchoke));
          cout << "could not choke peer with piece id" << forchoke << endl;
          break;
        }
      }
    }

    {
      std::shared_lock<std::shared_mutex> txforconnected_fds_map_con11(mtxforconnected_fds_map_con);
      std::unique_lock<std::shared_mutex> mtxforpi87(mtxforallchoked);
      //  std::unique_lock<std::shared_mutex> mtxforI_interested9(mtxforallinterested);
      // s//td::unique_lock<std::shared_mutex> mtxforfor_rarest13(mtxforfor_raresandgetrightidt);
      // std::shared_lock<std::shared_mutex> mtxforpeer_speed15(mtxforpeer_speed);
      //  std::shared_lock<std::shared_mutex> mtxforxih(mtxforpieces_ihave_ooyaehhh);
      //  std::shared_lock<std::shared_mutex> jhgjh(mtxforallchoked);
      connected_fds_map_con = connected_fds_map_con_copy_have;
      chocked_by_me = chocked_by_me_have;
      unchocked_by_me = unchocked_by_me_have;
      //  piece_info_have = piece_info;
      // interested_have = interested;
      // for_rarest_have = for_rarest;
      // peer_speed_have = peer_speed;
      // pieces_ihave_ooyaehhh_have = pieces_ihave_ooyaehhh;
      // I_interested_have = I_interested;
    }
  };
  void unchoke()
  {
    map<uint32_t, uint32_t> connected_fds_map_con_copy_have;
    // map<uint32_t, vector<uint32_t>> piece_info_have;
    vector<uint32_t> interested_have;
    vector<uint32_t> unchocked_by_me_have;
    vector<uint32_t> chocked_by_me_have;
    // vector<uint32_t> for_rarest_have;
    map<uint32_t, uint32_t> peer_speed_have;
    // vector<uint32_t> pieces_ihave_ooyaehhh_have;
    // vector<uint32_t> I_interested_have;
    {
      std::shared_lock<std::shared_mutex> txforconnected_fds_map_con11(mtxforconnected_fds_map_con);
      // std::unique_lock<std::shared_mutex> mtxforpiece_info987(mtxforallpiece_info);
      std::unique_lock<std::shared_mutex> mtxforI_interested9(mtxforallinterested);
      // s//td::unique_lock<std::shared_mutex> mtxforfor_rarest13(mtxforfor_raresandgetrightidt);
      std::shared_lock<std::shared_mutex> jhgjh(mtxforallchoked);
      std::shared_lock<std::shared_mutex> mtxforpeer_speed15(mtxforpeer_speed);
      //  std::shared_lock<std::shared_mutex> mtxforxih(mtxforpieces_ihave_ooyaehhh);

      connected_fds_map_con_copy_have = connected_fds_map_con;
      //  piece_info_have = piece_info;
      interested_have = interested;
      unchocked_by_me_have = unchocked_by_me;
      chocked_by_me_have = chocked_by_me;
      // for_rarest_have = for_rarest;
      peer_speed_have = peer_speed;
      // pieces_ihave_ooyaehhh_have = pieces_ihave_ooyaehhh;
      // I_interested_have = I_interested;
    }

    quick_sort forclassch;
    uint8_t send_state = snd_unchoke;
    uint8_t message_id = 1;
    uint32_t length_prefix = 1;
    uint32_t connectedfd;
    int length = 2;
    vector<uint32_t> forintspeed;
    // std::unique_lock<std::shared_mutex> sr7(mtxforsend);
    cout << "stepped into mtxsndrecv sr7 " << endl;
    // std::shared_lock<std::shared_mutex> mtxforconnected_fds_map_con5(mtxforconnected_fds_map_con);
    // std::shared_lock<std::shared_mutex> mtxforinterested1(mtxforallinterested);
    std::shared_lock<std::shared_mutex> mtxforunchoked_by_me4(mtxforfirstfive);
    if (first_5)
    {
      if (interested_have.size() != 0)
      {
        {
          // std::shared_lock<std::shared_mutex> mtxforpeer_speed6(mtxforpeer_speed);
          for (auto &itrtogetspeed : interested_have)
          {
            forintspeed.push_back(peer_speed_have[itrtogetspeed]); // get interested speeds
          }
          cout << "speed" << forintspeed.size() << endl;

          forclassch.quicksort(forintspeed, forintspeed.size(), interested_have);
        }
        for (uint32_t i = 0; i < 1; i++) // intersted ma rakhunga hu top three
                                         // speed vle pta ni kya likha ha mne ya
        {
          if (i >= interested_have.size() || interested_have.size() == 0)
          {
            break;
          }

          if (i > connected_fds_map_con_copy_have.size())
          {
            cout << "returinh" << endl;
            return;
          }
          connectedfd = connected_fds_map_con[*(interested_have.data() + i)];
          cout << "unchoking peer with piece id    " << *(interested_have.data() + i)
               << endl;
          unchocked_by_me_have.push_back(*(interested_have.data() + i));
          // auto start = high_resolution_clock::now();
          forunchokeobj.sendfxn_normal(message_id, connectedfd);
          // ch.sendfxn_req(piece_id, offset, length, connectedfd); // send
          // message to req the piece
          //  auto end = high_resolution_clock::now();
          // auto result = duration_cast<nanoseconds>(end - start);
          // int finalspeedint = length / result.count();
          // uint32_t finalspeed = (uint32_t)finalspeedint;
          //{
          //      std::unique_lock<std::shared_mutex>
          //      mtxforpeer_speed6(mtxforpeer_speed);
          //   peer_speed[*(interested.data() + i)] = finalspeed;
          //   }
          chocked_by_me_have.erase(std::remove(chocked_by_me_have.begin(), chocked_by_me_have.end(), *(interested_have.data() + i)));
        }
        if (interested_have.size() > 3)
        {
          int rndmport;
          uint32_t rndprt_uint;

          for (auto &bcpichachoda : interested_have)
          {
            rndmport = (8080 + (rand() % 1));
            rndprt_uint = uint32_t(rndmport);
            if (rndprt_uint != bcpichachoda)
            {
              connectedfd = connected_fds_map_con_copy_have[(rndprt_uint)];
              unchocked_by_me_have.push_back(rndprt_uint);
              forunchokeobj.sendfxn_normal(message_id, connectedfd);
              unchocked_by_me_have.erase(std::remove(unchocked_by_me_have.begin(), unchocked_by_me_have.end(), rndprt_uint));
              cout << "returning line 1201" << endl;
              return;
            }
          }
        };
      }
    }
    else
    {
      cout << "unchoking all peers" << endl;
      // if (first_5)
      //{
      for (auto &unchokeall : connected_fds_map_con_copy_have)
      {
        unchocked_by_me_have.push_back(unchokeall.first);

        connectedfd = unchokeall.second;
        cout << "unchokng  andsent msg unchoke to " << unchokeall.first << "connectedfd" << connectedfd << endl;
        // auto start = high_resolution_clock::now();
        forunchokeobj.sendfxn_normal(message_id, connectedfd);
        // ch.sendfxn_req(piece_id, offset, length, connectedfd // send
        // message to req the piece
        // auto end = high_resolution_clock::now();
        //  auto result = duration_cast<nanoseconds>(end - start);
        //  int finalspeedint = length / result.count();
        //  uint32_t finalspeed = (uint32_t)finalspeedint;
        // {
        //  std::unique_lock<std::shared_mutex> mtxforpeer_speed6(
        //        mtxforpeer_speed);
        //   peer_speed[unchokeall.first] = finalspeed;
        //  }
        if (chocked_by_me_have.size() != 0)
        {
          chocked_by_me_have.erase(std::remove(
              chocked_by_me_have.begin(), chocked_by_me_have.end(), unchokeall.first));
        }
      }
      // }
    }
    {
      std::shared_lock<std::shared_mutex> txforconnected_fds_map_con11(mtxforconnected_fds_map_con);
      std::unique_lock<std::shared_mutex> mtxforI_interested9(mtxforallinterested);
      std::shared_lock<std::shared_mutex> jhgjh(mtxforallchoked);
      std::shared_lock<std::shared_mutex> mtxforpeer_speed15(mtxforpeer_speed);
      // std::unique_lock<std::shared_mutex> mtxforpiece_info987(mtxforallpiece_info);

      // s//td::unique_lock<std::shared_mutex> mtxforfor_rarest13(mtxforfor_raresandgetrightidt);

      //  std::shared_lock<std::shared_mutex> mtxforxih(mtxforpieces_ihave_ooyaehhh);

      connected_fds_map_con = connected_fds_map_con_copy_have;
      //  piece_info_have = piece_info;
      interested_have = interested_have;
      chocked_by_me = chocked_by_me_have;
      unchocked_by_me = unchocked_by_me_have;
      // for_rarest_have = for_rarest;
      peer_speed_have = peer_speed;
      // pieces_ihave_ooyaehhh_have = pieces_ihave_ooyaehhh;
      // I_interested_have = I_interested;
    }
  }
  void unchoketop3peers()
  {
    cout << "unchoke caleed" << endl;
    unchoke();
    // cout << "choke caleed" << endl;
    // {
    //   std::shared_lock<std::shared_mutex> mtxforconnected_fds_map_con11(mtxforfirstfive);
    //   if (!first_5)
    //   {
    //    choke();
    //  }
    // }
  }
  void sortpeerbyuploadspeed()
  {
    cout << "sort called" << endl;
    quick_sort qs;
    // uint32_t length = sorted_arry.size();
    // qs.quicksort(sorted_arry, length, no_use);
  }
  void selectpeerstounchoke()
  {
    while (true)
    {
      cout << "choke algo started oo yeah" << endl;

      // sortpeerbyuploadspeed();
      unchoketop3peers();
      // while (chocked_by_me.size() < 2)
      //{
      //   sleep(1);
      // }
      // cout << "choke ended" << endl;
      sleep(10);
      // chokeallotherpeers();
    }
  }
};
// int MY_SOCKET1(const char *ip, uint16_t &port);
int MY_SOCKET1(const char *ip, uint32_t &port, struct sockaddr_in &address)
{
  int my_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (my_socket == -1)
    cout << "socket creation on ip failed" << ip << endl;
  else
    cout << "socket creation on ip completed" << ip << endl;

  address.sin_family = AF_INET;
  address.sin_port = htons(port);
  address.sin_addr.s_addr = INADDR_ANY;
  //  inet_pton(AF_INET, ip, &address.sin_addr);
  socklen_t sizeaddr = sizeof(address);
  int br = bind(my_socket, (sockaddr *)&address, sizeaddr);
  if (br == 0)
  {
    cout << "bind was successful at ip and port" << ip << " " << port << br
         << "hviv" << endl;
  }
  else
  {
    cout << "bind was not successful at ip and port" << ip << port << br
         << "hviv" << endl;
    return 1;
  }
  int lis = listen(my_socket, 11);
  if (lis == 0)
  {
    cout << "listening at port " << port << " ip " << ip << endl;
  }
  else
  {
    cout << "listening at port " << port << " ip " << ip << "failed" << endl;
  }
  return my_socket;
};
int main()
{
  for_rarest.clear();
  for_rarest.reserve(1000); // Reserve space to prevent frequent reallocations
  pieces_ihave_ooyaehhh.clear();
  piece_info.clear();
  piece_info_vec.clear();
  piece_info_reverse.clear();
  interested.clear();
  I_interested.clear();
  uninterested.clear();
  chocked_by_me.clear();
  chocked_by.clear();
  unchocked_by.clear();
  unchocked_by_me.clear();
  temport.clear();
  tempspeed.clear();
  getrightid = 0;
  rarest = 0;
  struct sockaddr_in local_address;
  int my_socket = MY_SOCKET1(my_ip, my_port, local_address);
  updater_cum_download ucp;
  connectionhandler ch1;
  my_jason torrent_cum_json;
  torrent_cum_json.my_jason1();
  file_hash_string =
      torrent_cum_json.sha1_calculator(info_hash, info_hash.length());
  file_hash = file_hash_string.c_str();
  string directory1 = "/home/kartik/Downloads/" + to_string(my_port) + "/";
  string directory2 =
      "/home/kartik/Downloads/" + to_string(my_port) + "ogfile" + "/";
  string directory3 =
      "/home/kartik/Downloads/dumpfile" + to_string(my_port) + "/";
  if (!ogseeder)
  {
    std::filesystem::create_directories(directory1);
    std::filesystem::create_directories(directory2);
    std::filesystem::create_directories(directory3);
  }
  sleep(15);
  cout << "peer0" << endl;
  Peer peer1(8081, "127.0.0.1", 1, 0, my_socket);
  sleep(10);
  /* cout << "peer1" << endl;
  Peer peer2(8082, "127.0.0.1", 2, 0);
  sleep(10);
  Peer peer3(8083, "127.0.0.1", 3, 0);
  sleep(10);
  Peer peer4(8084, "127.0.0.1", 4, 0);
  sleep(10);
  Peer peer5(8085, "127.0.0.1", 5, 0);
  // sleep(10);
  Peer peer6(8086, "127.0.0.1", 6, 0);
  // sleep(10);
  Peer peer7(8087, "127.0.0.1", 7, 0);
  // sleep(10);
  Peer peer8(8088, "127.0.0.1", 8, 0);
  // sleep(10);
 // Peer peer9(8089, "127.0.0.1", 9, 0);
  // sleep(10);
//  Peer peer10(8090, "127.0.0.1", 10, 0);
  // sleep(10);*/
  if (ogseeder)
  {
    std::shared_lock<std::shared_mutex> gdy(mtxforpieces_ihave_ooyaehhh);
    for (uint32_t i = 0; i < num_pieces; i++)
    {
      pieces_ihave_ooyaehhh.push_back(i);
    }
    cout << "pushed piecs for seeder" << endl;
  }
  while ((connected_fds_map_con.empty()))
  {
    std::shared_lock<std::shared_mutex> chok12(mtxforconnected_fds_map_con);
    sleep(1);
  }
  {
    cout << "2281" << endl;
    std::shared_lock<std::shared_mutex> confdatline1571atmain(mtxforconnected_fds_map_con);
    std::unique_lock<std::shared_mutex> confdatline1571an(mtxforallchoked);
    cout << "2281 start" << endl;
    for (auto &bokunohaha : connected_fds_map_con)
    {
      // uint_t send_state = snd_inerested;
      uint32_t length_prefix = 1;
      uint8_t message_id = 2;
      cout << "called sent at around 1576 for interested message should be 2" << message_id << endl;
      uint32_t connectedfd = bokunohaha.second;
      cout << "called sent at around 1576 for interested message should be 2" << message_id << connectedfd << endl;
      cout << "called sent at around 1576 for interested message should be 2" << message_id << endl;
      //  ch1.sendfxn_normal(message_id, connectedfd);
      message_id = 1;
      ch1.sendfxn_normal(message_id, connectedfd);
      unchocked_by_me.push_back(bokunohaha.first);
    }
  };
  chokealgo w1;
  updater_cum_download w2;
  updater_cum_download w3;
  cout << "starting thread for choke" << endl;
  while (unchocked_by.empty())
  {
    std::shared_lock<std::shared_mutex> chok12(mtxforallchoked);
    sleep(1);
  }
  // thread(&chokealgo::selectpeerstounchoke, &w1).detach();

  while ((unchocked_by.empty()))
  {
    std::shared_lock<std::shared_mutex> allchokemain(mtxforallchoked);
    sleep(1);
  }
  if (!ogseeder)
  {
    cout << "wait ended in main for choked " << endl;
    thread(&updater_cum_download::first_five, &w2).join();
    cout << "first five" << endl;
    // thread(&updater_cum_download::other_pieces, &w3).detach();
    // cout << "otehr piece" << "called" << endl;
  }
  cout << "starting while for check for_rarest.size() " << endl;
  while (for_rarest.size() == 0)
  {
    std::shared_lock<std::shared_mutex> abc(mtxforfor_raresandgetrightidt);
    // std::shared_lock<std::shared_mutex> abcd(mtxforpieces_ihave_ooyaehhh);
    sleep(1);
  }
  cout << "for rarest ended " << for_rarest.size() << endl;
  cout << " starting while for piece_info.size() " << endl;
  while (piece_info.size() == 0 && piece_info_reverse.size() == 0)
  {
    std::shared_lock<std::shared_mutex> abcde(mtxforallpiece_info);
    // std::shared_lock<std::shared_mutex> abcdef(mtxforpiece_info_reverse);
    sleep(1);
  }
  cout << "piece_info.size() ended " << piece_info.size() << endl;
  cout << "starting while for piece_info_vec.size() " << endl;
  while (piece_info_vec.size() == 0 && interested.size() == 0)
  {
    std::shared_lock<std::shared_mutex> abcdefg(mtxforallpiece_info);
    std::shared_lock<std::shared_mutex> abcdefgh(mtxforallinterested);
    sleep(1);
  }
  cout << "piece_info_vec.size() ended " << piece_info_vec.size() << endl;
  cout << "I_interested.size() " << endl;
  while (I_interested.size() == 0 && uninterested.size() == 0)
  {
    std::shared_lock<std::shared_mutex> abcdefghi(mtxforallinterested);
    // std::shared_lock<std::shared_mutex> abcdefghl(mtxforuninterested);
    sleep(1);
  }
  cout << "I_interested.size() ended " << I_interested.size() << endl;

  // //while (temport.size() == 0 && tempspeed() == 0)
  // {
  // std::shared_lock<std::shared_mutex> rfguh1(mtxfortemport);
  // std::shared_lock<std::shared_mutex> rfguh(mtxfortempspeed);
  //   sleep(1);
  // }

  cout << "startign loop for checking if i have all pieces" << endl;
  while (pieces_ihave_ooyaehhh.size() < num_pieces)
  {
    sleep(1);
  }
  cout << "pices count check ended" << endl;
  cout << "starting writing og file fxn" << endl;
  peer1.filereader_cum_cumbiner();
  cout << "file writer fxn finished" << endl;
  cin.get();
  string directory1cpy =
      "/home/kartik/Downloads/" + to_string(my_port) + "/.temp";
  string directory2cpy =
      "/home/kartik/Downloads/" + to_string(my_port) + "ogfile" + "/";
  string directory3cpy =
      "/home/kartik/Downloads/dumpfile" + to_string(my_port) + "/";
  std::remove(directory1cpy.c_str());
  std::remove(directory3cpy.c_str());
  return 0;
} // main ends
