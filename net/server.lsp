(do
  (import "net/net")
  (= S (net-newStream))
  ; (net-register S)

  (= onLine (fn (udata stm rem msg data sz)
    (printf "line: (%s %s) -> %s" 
            (net-getAddress stm) 
            (net-getPort stm) data)))

  (= onError (fn (udata stm rem msg data sz)
    (printf "error: (%s %s) -> %s" 
            (net-getAddress stm) 
            (net-getPort stm) msg)))

  (= onAccept (fn (udata stm rem msg data sz)
    (net-addListener rem "line" onLine rem)
    (printf "accept: (%s %s) -> %s" 
            (net-getAddress stm) 
            (net-getPort stm) msg)))

  (net-addListener S "line" onLine nil)
  (net-addListener S "error" onError nil)
  (net-addListener S "accept" onAccept nil)

  (net-listen S "localhost" 8000)

  (while (> (net-getStreamCount) 0)
    (net-update)))
