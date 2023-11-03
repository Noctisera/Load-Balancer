# README Load Balancer

## Description
This a program that simulates a Load Balancer using Consistent Hasing. Its role is to make sure items are being distributed evenly across servers. It also has the advantage of minimizing the number of transfers when one server is shut down or added.


## What does it do?
It has multiple commands that the admin can use to manage the servers and the database of items.

1.	add_server {server_id} - Add 3 replicas for the server on the hash ring. The servers will be added using a hash function to make sure they are evenly distributed on the hash ring.
2.	remove_server {server_id} - Remove the server with the chosen id and all its replicas. All the items stored in them will be moved to the next server on the hash ring.
3.	store {item_key} {item_value} - Introduces the key in a hash function that returns an address on the hash ring, then puts the item in the server with the lowest address and higher than its.
4.	retrieve {item_key} - Finds the server that the item is stored on and returns the item_value.

## Code structure and implementation
The hash ring that the servers are stored on is a vector that changes its size when adding or removing servers, saving memory this way. There is a second vector that saves the address of each server and with the help of it, the servers are always in order after an addition or removal.

Adding a server increases the size of the hash ring by 3, adds the 3 replicas at the end of it, also increases the size of the addresses vector, adds their addresses and then sorts them. Then each newly added servers recieves the items from its higher neightbour that have a lower address than the new server has.

Removing a server (unexpectedly) decreases the size of the hash ring by 3 and transfers all their items on the servers following them on the hash ring.

Items are stored inside hashtables that each servers has with linear probing (kinda).

Once an item is stored, it can't be deleted back. Only retrieved. (They are deleted only when the program ends and all the allocated memory is freed)

## Errors
Errors will appear most likely in the following scenarios so make sure to avoid them:
- Removing all the servers
- Adding servers with huge a server_id
- Adding a humongous numbers of items in few servers (each server has a maximum hashtable size of 10000 entries)


### What could have been done better
I made a second vector just for the addresses of the servers when I could have just saved them in the server_memory struct in a new variable. This implementation was added when I thought the server vector also needed to be a hashtable and I was too lazy to make them into only one.

Using binary search to find the servers where the items had to be stored. The complexity would drop from O(n) to O(log(n)) this way.

Using chaining instead of linear probing could have probably been better since infinite items could have been added on each server.
