## BUILD REMOTE CONTAINER
```shell
cd <PATH>/grpcpp

docker build \
    -t grpcpp-dev \
    -f container/dockerfile \
    .
```

## RUN REMOTE CONTAINER
```shell
cd <PATH>/grpcpp

docker run \
    -d \
    -it \
    -v $(pwd):/app \
    --net=host \
    --name grpcpp-dev \
    grpcpp-dev 
```

## STOP REMOTE CONTAINER
```shell
docker stop grpcpp-dev && docker rm grpcpp-dev
```

## STEP IN REMOTE CONTAINER
```shell
docker exec -it grpcpp-dev /bin/bash
```

## ATTACH TO REMOTE CONTAINER
1) W: Create SSH priv/pub key pair on 
    ```
    ssh-keygen -t rsa -b 4096 -f c:\mk\ssh_keys\id_rsa_nuc245 -P ""
    ```

2) W: Open CMD in admin mode and start SSH agent
    ```
    sc config ssh-agent start=auto
    net start ssh-agent
    ssh-add c:\mk\ssh_keys\id_rsa_nuc245
    ```

3) W: Copy public key to target
    ```
    type c:\mk\ssh_keys\id_rsa_nuc245.pub | ssh mkasap@192.168.1.245 "cat >> .ssh/authorized_keys"
    ```

4) W: Test if can connect:
    ```
    ssh -i c:\mk\ssh_keys\id_rsa_nuc245 mkasap@192.168.1.245
    ```
5) W: Create remote docker context
    ```
    docker context create nuc245-docker-machine --docker "host=ssh://mkasap@192.168.1.245:22"
    ```

6) In VSCode settings, select docker.context settings and fill with value:
    ```
    @ext:ms-azuretools.vscode-docker docker.context : nuc245-docker-machine
    ```

7) Issue the Docker Context, Command Palette (Ctrl+Shift+P)
    ```
    Docker Contexts: Use
        select default context
    ```

8) Command Palette (Ctrl+Shift+P)
    ```
    remote-containers.attachToRunningContainer
    ```
    
9) In VSCode settings, select docker.context settings and fill with value:
    ```
    docker.explorerRefreshInterval: 3000
    ```

## VSCode Debug
- Install C++ extension pack to VSCode 
    https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools-extension-pack

- C++ Ext Pack will install CMake extension.

- Update CMake settings so it will point to app directory:
```
cmake.sourceDirectory
```

- In CMakeFile.txt root folder, on the status select the kit and compile with CMake icon on the status bar.
    Kit i.e.: [GCC 9.3.0 X86_64-linux-gnu]


- Create launch.json:
```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "laa Remote Debug (C)",
      "type": "cppdbg",
      "request": "attach",
      "program": "./main",
      "processId": "${command:pickRemoteProcess}",
      "pipeTransport": {
        "pipeCwd": "${workspaceFolder}",
        "pipeProgram": "docker",
        "pipeArgs": [
          "exec",
          "-i",
          "laa",
          "sh",
          "-c"
        ],
        "debuggerPath": "/usr/bin/gdb"
      },
      "sourceFileMap": {
        "/app": "${workspaceFolder}/modules/laa"
      },
      "linux": {
        "MIMode": "gdb",
        "setupCommands": [
          {
            "description": "Enable pretty-printing for gdb",
            "text": "-enable-pretty-printing",
            "ignoreFailures": true
          }
        ]
      },
      "osx": {
        "MIMode": "lldb"
      },
      "windows": {
        "MIMode": "gdb",
        "setupCommands": [
          {
            "description": "Enable pretty-printing for gdb",
            "text": "-enable-pretty-printing",
            "ignoreFailures": true
          }
        ]
      }
    }
  ]
}
```

## VSCODE REMOTE SSH CONN
- Create config file under:
    C:\Users\mkasap.REDMOND\.ssh
- cat config
    Host NUC245
        HostName 192.168.1.245
        User mkasap
        PubKeyAuthentication yes
        IdentitiesOnly yes
        IdentityFile C:\mk\ssh_keys\id_rsa_nuc245
