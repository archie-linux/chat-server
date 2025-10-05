### Run Server

- gcc -o server server.c
- ./server

### Connect to server via netcat

- nc 127.0.0.1 8080

- Message format: [Recipient's Username] [Message To Send]

### Demo

#### Server

<pre>
MacBook-Air:chat-server anish$ gcc -o server server.c
MacBook-Air:chat-server anish$ ./server 
Server listening on port 8080
Client connected - IP address: [127.0.0.1] Port: [64016]
Client registered: bob
Client connected - IP address: [127.0.0.1] Port: [64017]
Client registered: alice
Client connected - IP address: [127.0.0.1] Port: [64045]
Client registered: tim
Client disconnected
Deregistered client: alice
Client connected - IP address: [127.0.0.1] Port: [64116]
Client registered: john
^C
</pre>


#### User 1

<pre>
MacBook-Air:chat-server anish$ nc 127.0.0.1 8080
Please enter your username: bob
Registration successful. Welcome to the chat server!
alice hi
alice: hey
alice how are you?
alice: I am good
tim: How have you been?
tim Great. How about you?
alice: I have to leave. Bye!
alice okay. bye
</pre>

#### User 2

<pre>
MacBook-Air:chat-server anish$ nc 127.0.0.1 8080
Please enter your username: alice
Registration successful. Welcome to the chat server!
bob: hi
bob hey
bob: how are you?
bob I am good
bob I have to leave. Bye!                  
bob: okay. bye
^C
</pre>

#### User 3

<pre>
MacBook-Air:chat-server anish$ nc 127.0.0.1 8080
Please enter your username: tim
Registration successful. Welcome to the chat server!
bob How have you been?    
bob: Great. How about you?
</pre>


#### User 4

<pre>
MacBook-Air:chat-server anish$ nc 127.0.0.1 8080
Please enter your username: john
Registration successful. Welcome to the chat server!
hi 
Recipient not found
</pre>