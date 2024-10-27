# Video sharing program
This is a video sharing program that consists of a server, who stores all the videos and a client that downloads them. Both machines must be directly connected by an Ethernet cable as we have implemented our own Link layer protocol similar to the Ethernet protocol or Kermit protocol.  
This was implemented using only Raw Sockets in promiscuous mode, no TCP or UDP for addressing.

We limited to .mp4 and .avi files but this same program could theoretically share any sort of file.  

# How to use
* Connect both computers via an Ethernet Cable.
* Open the Client.c or Server.c file:
  * Run the ifconfig command to check the ethernet interface
  * change the ```interface[]="eth0";``` inside the main() functions
* To compile the programs run:  
  * ```make client```  
  * ```make server```
## Running the client
  * You can choose between:
    * Asking for the list of available videos.
    * Asking to download a specific video.
      * To choose a video you must type the file name.
## Running the server
  * To setup the videos, you must include the files in the videos folder
    * Keep in mind you are limited to .mp4 and .avi
  * The server, while open, will simply wait for a request.

# The protocol
## The Frame
![image](https://github.com/user-attachments/assets/4f8a8614-58a5-48f4-a8a9-6b7d1c82c666)


### Flow control:
* Sliding windows Go-Back-N of fixed length 5
### Error detection:
* CRC-8
