
;;; minime.scm -- minime library procedures

(display "Welcome to minime, happy hacking!")
(newline)


;; simple version of map until optional arguments are in
(define (map proc lst)
  (if (null? lst)
      '()
      (cons (proc (car lst))
	    (map proc (cdr lst)))))



(define (fold-left op initial sequence)
  (define (iter result rest)
    (if (null? rest)
	result
	(iter (op result (car rest))
	      (cdr rest))))
  (iter initial sequence))

(define (fold-right op initial sequence)
  (if (null? sequence)
      initial
      (op (car sequence)
	  (fold-right op initial (cdr sequence)))))

(define accumulate fold-right)


