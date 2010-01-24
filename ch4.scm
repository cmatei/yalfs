
;;
;; Meta-circular evaluator from "Structure and Interpretation of Computer Programs"
;;
;; It is NOT covered by the GPL. Original code from the book can be found here
;; http://mitpress.mit.edu/SICP/code/index.html

(define eval-in-underlying-scheme eval)
(define apply-in-underlying-scheme apply)

;; EVAL

(define (eval exp env)
  (cond ((self-evaluating? exp) exp)
	((variable? exp) (lookup-variable-value exp env))
	((quoted? exp) (text-of-quotation exp))
	((assignment? exp) (eval-assignment exp env))
	((definition? exp) (eval-definition exp env))
	((if? exp) (eval-if exp env))
	((lambda? exp)
	 (make-procedure (lambda-parameters exp)
			 (lambda-body exp)
			 env))
	((begin? exp)
	 (eval-sequence (begin-actions exp) env))
	((cond? exp)
	 (eval (cond->if exp) env))
	((and? exp)
	 (eval-and (operands exp) env))
	((or? exp)
	 (eval-or (operands exp) env))
	((let? exp)
	 (eval (let->combination exp) env))
	((let*? exp)
	 (eval (let*->let exp) env))
	((application? exp)
	 (apply (eval (operator exp) env)
		(list-of-values (operands exp) env)))
	(else
	 (error "Unknown expression type -- EVAL" exp))))

;; APPLY

(define (apply procedure arguments)
  (cond ((primitive-procedure? procedure)
	 (apply-primitive-procedure procedure arguments))
	((compound-procedure? procedure)
	 (eval-sequence (procedure-body procedure)
			(extend-environment
			 (procedure-parameters procedure)
			 arguments
			 (procedure-environment procedure))))
	(else
	 (error "Unknown procedure type -- APPLY" procedure))))

;; procedure arguments

(define (list-of-values exps env)
  (if (no-operands? exps)
      '()
      (cons (eval (first-operand exps) env)
	    (list-of-values (rest-operands exps) env))))

;; conditionals

(define (eval-if exp env)
  (if (true? (eval (if-predicate exp) env))
      (eval (if-consequent exp) env)
      (eval (if-alternative exp) env)))

;; sequences

(define (eval-sequence exps env)
  (cond ((last-exp? exps) (eval (first-exp exps) env))
	(else
	 (eval (first-exp exps) env)
	 (eval-sequence (rest-exps exps) env))))

;; assignment and definition

(define (eval-assignment exp env)
  (set-variable-value! (assignment-variable exp)
		       (eval (assignment-value exp) env)
		       env)
  'ok)

(define (eval-definition exp env)
  (define-variable! (definition-variable exp)
                    (eval (definition-value exp) env)
		    env)
  'ok)

;; ex 4.4

(define (and? exp) (tagged-list? exp 'and))
(define (eval-and exp env)
  (cond ((null? exp) true)
	((eq? (eval (car exp) env) false) false)
	(else
	 (eval-and (cdr exp) env))))

(define (or? exp) (tagged-list? exp 'or))
(define (eval-or exp env)
  (cond ((null? exp) false)
	((eq? (eval (car exp) env) true) true)
	(else
	 (eval-or (cdr exp) env))))

;; 4.1.2 representing expressions

(define (self-evaluating? exp)
  (cond ((number? exp) true)
	((string? exp) true)
	(else false)))

(define (variable? exp) (symbol? exp))

(define (quoted? exp)
  (tagged-list? exp 'quote))

(define (text-of-quotation exp) (cadr exp))

(define (tagged-list? exp tag)
  (if (pair? exp)
      (eq? (car exp) tag)
      false))

(define (assignment? exp)
  (tagged-list? exp 'set!))

(define (assignment-variable exp) (cadr exp))
(define (assignment-value exp) (caddr exp))

(define (definition? exp)
  (tagged-list? exp 'define))

;; (define (var param1 ...) body) is syntactic sugar fo
;; (define var (lambda (param1...) (body)))

(define (definition-variable exp)
  (if (symbol? (cadr exp))
      (cadr exp)
      (caadr exp)))

(define (definition-value exp)
  (if (symbol? (cadr exp))
      (caddr exp)
      (make-lambda (cdadr exp) ;; formal params
		   (cddr exp)))) ;; body


(define (lambda? exp) (tagged-list? exp 'lambda))

(define (lambda-parameters exp) (cadr exp))
(define (lambda-body exp) (cddr exp))

(define (make-lambda parameters body)
  (cons 'lambda (cons parameters body)))

(define (if? exp) (tagged-list? exp 'if))

(define (if-predicate exp) (cadr exp))
(define (if-consequent exp) (caddr exp))
(define (if-alternative exp)
  (if (not (null? (cdddr exp)))
      (cadddr exp)
      'false))

(define (make-if predicate consequent alternative)
  (list 'if predicate consequent alternative))

(define (begin? exp) (tagged-list? exp 'begin))
(define (begin-actions exp) (cdr exp))
(define (last-exp? seq) (null? (cdr seq)))
(define (first-exp seq) (car seq))
(define (rest-exps seq) (cdr seq))

(define (sequence->exp seq)
  (cond ((null? seq) seq)
	((last-exp? seq) (first-exp seq))
	(else (make-begin seq))))

(define (make-begin seq) (cons 'begin seq))


(define (application? exp) (pair? exp))

(define (operator exp) (car exp))

(define (operands exp) (cdr exp))

(define (no-operands? ops) (null? ops))

(define (first-operand ops) (car ops))

(define (rest-operands ops) (cdr ops))

;; derived expressions - expressions implemented
;; as syntactic transformations

;; cond is transformed into nested ifs

(define (cond? exp) (tagged-list? exp 'cond))

(define (cond-clauses exp) (cdr exp))

(define (cond-else-clause? clause)
  (eq? (cond-predicate clause) 'else))

(define (cond-predicate clause) (car clause))

(define (cond-actions clause) (cdr clause))

(define (cond->if exp)
  (expand-clauses (cond-clauses exp)))

(define (expand-clauses clauses)
  (if (null? clauses)
      'false
      (let ((first (car clauses))
	    (rest (cdr clauses)))
	(if (cond-else-clause? first)
	    (if (null? rest)
		(sequence->exp (cond-actions first))
		(error "ELSE clause isn't last -- COND->IF" clauses))
	    (make-if (cond-predicate first)
		     (sequence->exp (cond-actions first))
		     (expand-clauses rest))))))

;; 4.2
;; a) x will be evaluated before being defined
;; b)

;;(define (application? exp) (tagged-list? exp 'call))
;;(define (operator exp) (cadr exp))
;;(define (operands exp) (cddr exp))

;; 4.5
;; evaluating cond-predicate twice. Not good, hackers.

(define (cond-extended-clause? clause)
  (eq? (cadr clause) '=>))

(define (extended-action->exp clause)
  (list 'apply (cadr (cond-actions clause)) (cond-predicate clause)))


(define (expand-clauses clauses)
  (if (null? clauses)
      'false
      (let ((first (car clauses))
	    (rest (cdr clauses)))
	(if (cond-else-clause? first)
	    (if (null? rest)
		(sequence->exp (cond-actions first))
		(error "ELSE clause isn't last -- COND->IF" clauses))
	    (make-if (cond-predicate first)
		     (if (cond-extended-clause? first)
			 (extended-action->exp first)
			 (sequence->exp (cond-actions first)))
		     (expand-clauses rest))))))

(cond->if '(cond (5 => display) (else (display "foo"))))

;; 4.6

(define (let? exp) (tagged-list? exp 'let))

(define (let-variables exp) (cadr exp))
(define (let-body exp) (cddr exp))

(define (let-variable-name varexp) (car varexp))
(define (let-variable-exp varexp)
  (if (null? (cdr varexp))
      '()
      (cadr varexp)))

(define (let->combination exp)
  (append
   (list
    (make-lambda (map let-variable-name (let-variables exp))
		 (let-body exp)))
   (map let-variable-exp (let-variables exp))))

;; (let->combination '(let ((x 1) (y (+ 2 3))) (display x) (display y)))

(let->combination '(let ((x 1) (y (+ 2 3))) (display y)))

;; 4.7

(define (let*? exp) (tagged-list? exp 'let*))

(define (let*->let exp)
  (define (make-lets exps)
    (append (list 'let (list (list (let-variable-name (car exps))
				   (let-variable-exp  (car exps)))))
	    (if (null? (cdr exps))
		(let-body exp)
		(list (make-lets (cdr exps))))))

  (make-lets (let-variables exp)))

;;(define x '(let* ((x) (y (* x x))) (display y)))
;;(let*->let x)
;;(eval (let*->let x) user-initial-environment)

(let*->let '(let* ((x 3)
		   (y (+ x 2))
		   (z (+ x y 5)))
	      (* x z)))


;; evaluator data structures

(define (true? x)
  (not (eq? x false)))

(define (false? x)
  (eq? x false))

;; representing procedures

(define (make-procedure parameters body env)
  (list 'procedure
	parameters
	(scan-out-defines body)
	env))

(define (compound-procedure? p)
  (tagged-list? p 'procedure))

(define (procedure-parameters p) (cadr p))

(define (procedure-body p) (caddr p))

(define (procedure-environment p) (cadddr p))

;; 4.16

(map (lambda (v) (list (car v) '*unassigned*)) (list '(a 1)))

(define (scan-out-defines body)
  (let ((pbody '())
	(pdefines '()))

    (define (list-of-bindings)
      (map (lambda (v) (list (definition-variable v) '(quote *unassigned*)))  pdefines))

    (define (list-of-set!)
      (map (lambda (v) (list 'set! (definition-variable v) (definition-value v))) pdefines))

    (define (scan-output)
      (let ((the-vars (list-of-bindings))
	    (the-sets (list-of-set!)))

	(if (null? the-vars)
	    pbody
	    (list (cons 'let (cons the-vars (append the-sets pbody)))))))

    (define (scan-body-exps exps)
      (cond ((null? exps) (scan-output))
	    ((definition? (car exps))
	     (set! pdefines (append pdefines (list (car exps))))
	     (scan-body-exps (cdr exps)))
	    (else
	     (set! pbody (append pbody (list (car exps))))
	     (scan-body-exps (cdr exps)))))

    (scan-body-exps body)))

;;(scan-out-defines '((define (v x) (+ x 5)) (define u 5) (v 1)))

;; operations on environments

;; list of frames. enclosing environment is the cdr of the list

(define (enclosing-environment env) (cdr env))

(define (first-frame env) (car env))

(define the-empty-environment '())

;; frames are pairs of lists, the variables bound in that frame and
;; the associated values

(define (make-frame variables values)
  (cons variables values))

(define (frame-variables frame) (car frame))

(define (frame-values frame) (cdr frame))

(define (add-binding-to-frame! var val frame)
  (set-car! frame (cons var (car frame)))
  (set-cdr! frame (cons val (cdr frame))))

(define (extend-environment vars vals base-env)
  (if (= (length vars) (length vals))
      (cons (make-frame vars vals) base-env)
      (error "wrong list lengths in extend-environment" vars vals)))

(define (lookup-variable-value var env)
  (define (env-loop env)
    (define (scan vars vals)
      (cond ((null? vars)
	     (env-loop (enclosing-environment env)))
	    ((eq? var (car vars))
	     (if (eq? (car vals) '*unassigned*)
		 (error "Unassigned variable" var)
		 (car vals)))
	    (else (scan (cdr vars) (cdr vals)))))
    (if (eq? env the-empty-environment)
	(error "Unbound variable" var)
	(let ((frame (first-frame env)))
	  (scan (frame-variables frame)
		(frame-values frame)))))

  (env-loop env))


(define (set-variable-value! var val env)
  (define (env-loop env)
    (define (scan vars vals)
      (cond ((null? vars)
	     (env-loop (enclosing-environment env)))
	    ((eq? var (car vars))
	     (set-car! vals val))
	    (else (scan (cdr vars) (cdr vals)))))

    (if (eq? env the-empty-environment)
	(error "Unbound variable -- SET!" var)
	(let ((frame (first-frame env)))
	  (scan (frame-variables frame)
		(frame-values frame)))))

  (env-loop env))

(define (define-variable! var val env)
  (let ((frame (first-frame env)))
    (define (scan vars vals)
      (cond ((null? vars)
	     (add-binding-to-frame! var val frame))
	    ((eq? var (car vars))
	     (set-car! vals val))
	    (else (scan (cdr vars) (cdr vals)))))
    (scan (frame-variables frame)
	  (frame-values frame))))


;;; running the evaluator as a program

(define primitive-procedures
  (list (list 'car car)
	(list 'cdr cdr)
	(list 'cons cons)
	(list 'null? null?)
	(list '= =)
	(list '* *)
	(list '+ +)
	(list '/ /)
	(list '- -)

	))

(define (primitive-procedure-names)
  (map car primitive-procedures))

(define (primitive-procedure-objects)
  (map (lambda (proc) (list 'primitive (cadr proc)))
       primitive-procedures))

(define (primitive-procedure? proc)
  (tagged-list? proc 'primitive))

(define (primitive-implementation proc) (cadr proc))

(define (setup-environment)
  (let ((initial-env
	 (extend-environment (primitive-procedure-names)
			     (primitive-procedure-objects)
			     the-empty-environment)))
    (define-variable! 'true true initial-env)
    (define-variable! 'false false initial-env)

    initial-env))

(define the-global-environment (setup-environment))

(define ge the-global-environment)



;; restore sanity
;;(define eval eval-in-underlying-scheme)
;;(define apply apply-in-underlying-scheme)

(define (apply-primitive-procedure proc args)
  (apply-in-underlying-scheme (primitive-implementation proc) args))

(define input-prompt ";;; M-Eval input: ")
(define output-prompt ";;; M-Eval value: ")

(define (driver-loop)
  (prompt-for-input input-prompt)
  (let ((input (read)))
    (let ((output (eval input the-global-environment)))
      (announce-output output-prompt)
      (user-print output)))
  (driver-loop))

(define (prompt-for-input string)
  (newline) (newline) (display string) (newline))

(define (announce-output string)
  (newline) (display string) (newline))


(define (user-print object)
  (if (compound-procedure? object)
      (display (list 'compound-procedure
		     (procedure-parameters object)
		     (procedure-body object)
		     '<procedure-env>))
      (display object)))


;;(driver-loop)


;; 4.18 && 4.19

;;   (lambda <vars>
;;     (let ((u '*unassigned*)
;;  	 (v '*unassigned*))
;;       (set! u <e1>)
;;       (set! v <e2>)
;;
;;       <e3>))
;;
;;  vs.
;;
;;    (lambda <vars>
;;      (let ((u '*unassigned*)
;;  	  (v '*unassigned*))
;;        (let ((a <e1>)
;;  	    (b <e2>))
;;  	(set! u a)
;;  	(set! v b))
;;
;;  	<e3>))
;;
;;  In the first case, e1 and e2 are evaluated sequentially, in the body
;;  where u and v are bound.
;;
;;    ((lambda (u v)
;;      (set! u <e1>)
;;      (set! v <e2>)
;;
;;      <e3>) '*unassigned* '*unassigned*)
;;
;;  This means that v (e2) can use the actual value of u (e1) by the time
;;  it is evaluated.
;;
;;  On the contrary, in the second case:
;;
;;    ((lambda (u v)
;;       ((lambda (a b)
;;  	(set! u a)
;;  	(set! v b)) <e1> <e2>)
;;
;;       <e3>) '*unassigned* '*unassigned*)
;;
;;  e1 and e2 are evaluated before the application of (lambda (a b) ...),
;;  which means that they see '*unassigned* for the values of u and v respectively, and
;;  an attempt to use such value will signal an error.

;; 4.18 - I don't think it will work because of (define dy (stream-map f y)) which will
;; want to apply f to the first value of the y stream so it can cons-stream the result.

;; 4.19 - Eva's way: wall-eeeee... nu stiu. ("It seems difficult to
;; implement a general, efficient mechanism that does what Eva
;; requires")


;; 4.21 - Y operator



(eval '(define (factorial n)
	  (if (= n 1)
	      1
	      (* n (factorial (- n 1))))) the-global-environment)

'loaded



