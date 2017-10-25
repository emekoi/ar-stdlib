(do
  (import "net/net")
  (= C (net-newStream))
  (net-register C)
  (net-connect C "localhost" 8000)
  (while (> (net-getStreamCount) 0)
    (net-update)
    (net-write C (read))))
