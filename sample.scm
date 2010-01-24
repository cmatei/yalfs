
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


42


(map (lambda (x) (if (= 100 x) (break))) '(1 2 3 4  6 7 8 9 100))

