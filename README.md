A simple C program based on the client-server model, which makes use of inter-process communication mechanisms such as pipes, fifos and socketpairs. The following commands can be executed:
- "login : username" (currently recognized users are listed in the **users.txt** file);
- "get-logged-users" - displays information (username, hostname for remote login, time entry was made) about all users that are logged in the operating system. User must be logged in to execute this command;
- "get-proc-info : pid" - displays information (name, state, ppid, uid, vmsize) about the process whose pid is specified. User must be logged in to execute this command;
- "logout";
- "quit".
