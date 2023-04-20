# Wireless Simulation

Wireless Simulation(ns3) ,Second CA of The Computer Networks course @ University of Tehran, spring 2023

Works fine with [ns-3.35](https://www.nsnam.org/releases/ns-3-35/)!


# Report





# What we did

In this assignment, we implemented two headers:

- `DecodedHeader`: The master (central) node uses this to send decoded messages received from the client to the workers (mappers). It also attaches the client's port and IP address to this header so that the workers can figure out how to communicate with the client and deliver the result.
- `EncodedHeader`: This header is used to deliver the associated character to the client from one of the workers.

We then implemented the worker (mapper) class so that it can receive data from the master, process (encode) it, and deliver the result to the client. For this purpose, we used the same UDP approach that was used for client->master communication for worker->client communication.

For Master->Workers communication, we used the TCP protocol, and for each worker, we have an independent TCP server, and we pass the Inet-address to the server to connect to them.

After receiving each packet (decoded data) from the client, the master creates a DecodedHeader to send the data+client address to workers. This is done sequentially. By using the same port for all workers, the master doesn't need to loop over clients and generate a packet each time.
