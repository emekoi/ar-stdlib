(do
  (import "net/net")
  (= S (net-newStream))

  (= onLine (fn ()
    (printf "line: (%s %s) -> %s" 
      (net-getAddress stream) 
      (net-getPort stream) data)))

  (= onError (fn ()
    (printf "error: (%s %s) -> %s" 
      (net-getAddress stream) 
      (net-getPort stream) msg)))

  (= onAccept (fn ()
    (net-addListener remote "line" onLine remote)
    (printf "accept: (%s %s) -> %s" 
      (net-getAddress stream) 
      (net-getPort stream) msg)))

  (net-addListener S "line" onLine nil)
  (net-addListener S "error" onError nil)
  (net-addListener S "accept" onAccept nil)

  (net-listen S "localhost" 8000)

  (while (> (net-getStreamCount) 0)
    (net-update)))
