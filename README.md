# Video sharing program
This is a video sharing program that consists of a server, who stores all the videos and a client that downloads them. Both machines must be directly connected by an Ethernet cable as we have implemented our own [Data link layer](https://en.wikipedia.org/wiki/OSI_model) protocol similar to the [Ethernet protocol](https://en.wikipedia.org/wiki/Ethernet) or [Kermit protocol](https://en.wikipedia.org/wiki/Kermit_(protocol)).  
This was implemented using only [Raw Sockets](https://www.baeldung.com/cs/raw-sockets) in promiscuous mode, no TCP or UDP for addressing.

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

### Timeout
* Implemented using [select()](https://man7.org/linux/man-pages/man2/select.2.html) checking for changes in the socket to avoid busy waiting.
  * [select()](https://man7.org/linux/man-pages/man2/select.2.html) was also used to check changes in stdin if user decided to interrupt the download.

### Flow control:
* Sliding window Go-Back-N of fixed length 5
  * With some small changes:
    * Added window in the client to save out of order packets (future packets after some got lost)
    * Attempts 16 retries:
      * After resetting the window 10 times, waits 1 second for each one (prevents wrong timeouts).
    * Sends an ACK every time it can, not only at the end of the window (works a lot faster this way).
    * Sender probes buffer every loop using [select()](https://man7.org/linux/man-pages/man2/select.2.html)
### Error detection:
* [CRC-8](https://en.wikipedia.org/wiki/Cyclic_redundancy_check)
