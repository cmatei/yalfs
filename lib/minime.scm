
;;; minime.scm -- minime library procedures

(display "Welcome to minime, happy hacking!")
(newline)


;; simple version of map until optional arguments are in
(define (map proc lst)
  (if (null? lst)
      '()
      (cons (proc (car lst))
	    (map proc (cdr lst)))))



