
;;
;; Example scheme code that can run under minime
;;
;; Run with: (load "sample.scm")
;;

(display "Hello, world!")
(newline)

(define (factorial n)
  (if (zero? n)
      1
      (* n (factorial (- n 1)))))

(display (list 10 'factorial '= (factorial 10)))
(newline)

(define (foo x . rest)
  (+ x (fold-left + 0 rest)))

(display (foo 10 20 30))
(newline)

42


