
;;; minime.scm -- minime library procedures

(display "Welcome to minime, happy hacking!")
(newline)


;; simple version of map until optional arguments are in
(define (map proc lst)
  (cond ((null? lst) '())
	(else (cons (proc (car lst))
		    (map proc (cdr lst))))))


