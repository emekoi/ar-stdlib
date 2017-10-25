(do
  (import "net/net")
  (= C (net-newStream))
  ;(net-register C)
  
  (= onLine (fn (udata stm rem msg data sz)
    (printf "line: (%s %s) -> %s" 
            (net-getAddress stm) 
            (net-getPort stm) data)))

  (= onError (fn (udata stm rem msg data sz)
    (printf "error: (%s %s) -> %s" 
            (net-getAddress stm) 
            (net-getPort stm) msg)))

  (= onConnect (fn (udata stm rem msg data sz)
    (printf "connect: (%s %s) -> %s" 
            (net-getAddress stm) 
            (net-getPort stm) msg)))

  (net-addListener S "line" onLine nil)
  (net-addListener S "error" onError nil)
  (net-addListener S "connect" onConnect nil)
  
  (net-connect C "localhost" 8000)
  (while (> (net-getStreamCount) 0)
    (net-update)
    (net-write C (read))))
