(do 
  (import "net/net")
  (= S (net-newStream))
  (net-register S)
  (net-listen S "localhost" 8000)
  (while (> (net-getStreamCount) 0) (net-update)))