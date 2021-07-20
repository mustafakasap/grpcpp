## Build docker image  

docker build \
    -t grpcpp-dev \
    -f dockerfile \
    .

## Run docker container
docker run \
    -d \
    -it \
    -v $(pwd):/app \
    --net=host \
    --name grpcpp-dev \
    grpcpp-dev 

docker stop grpcpp-dev && docker rm grpcpp-dev

docker exec -it grpcpp-dev /bin/bash


# REMOTE CONTAINER CONNECTION

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
