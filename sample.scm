
;;
;; Example scheme code that can run under minime
;;

(display "Hello, world!")
(newline)

(define (factorial n)
  (if (zero? n)
      1
      (* n (factorial (- n 1)))))

(factorial 10)

