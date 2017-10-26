(do
  (import "net/net")
  (import "fs/fs")
  (= C (net-newStream))
  (fs-mount ".")
  
  (= onLine (fn ()
    (printf "line: (%s %s) -> %s" 
      (net-getAddress stream) 
      (net-getPort stream) data)))

  (= onError (fn ()
    (printf "error: (%s %s) -> %s" 
      (net-getAddress stream) 
      (net-getPort stream) msg)))

  (= onConnect (fn ()
    (printf "connect: (%s %s) -> %s" 
      (net-getAddress stream) 
      (net-getPort stream) msg)))

  (net-addListener C "line" onLine nil)
  (net-addListener C "error" onError nil)
  (net-addListener C "connect" onConnect nil)

  (= FILE (fs-read "net/client.lsp"))
  
  (net-connect C "localhost" 8000)

  (net-write C FILE)

  (while (> (net-getStreamCount) 0)
    (net-update)