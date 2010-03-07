
;;; minime.scm -- minime library procedures

(display "Welcome to minime, happy hacking!")
(newline)


(define (map proc . lst)
  (define (map-one proc lst)
    (if (null? lst)
	'()
	(cons (proc (car lst))
	      (map-one proc (cdr lst)))))

  (define (map-many proc lst)
    (cond ((memq '() lst) '())
	  (else
	   (cons (apply proc (map-one car lst))
		 (map-many proc (map-one cdr lst))))))

  (map-many proc lst))

(define (for-each proc . lst)
  (define (for-each-many proc lst)
    (cond ((memq '() lst) '())
	  (else
	   (apply proc (map car lst))
	   (for-each-many proc (map cdr lst)))))

  (for-each-many proc lst))

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


;; delay & force. delay is syntax for
;; (delay <exp>) => (make-promise (lambda () <exp>))

(define (force object)
  (object))

(define (make-promise proc)
  (let ((result-ready? #f)
	(result #f))
    (lambda ()
      (if result-ready?
	  result
	  (let ((x (proc)))
	    (if result-ready?
		result
		(begin (set! result-ready? #t)
		       (set! result x)
		       result)))))))



