; 1) Связность 
; 2) Двудольность 
; 3) Эйлеровость 
;(╮°-°)╮┳━━┳ (╯°□°)╯ ┻━━┻
;Just do it! - coffee, yay!

;Стартуем!
(defun start(g)
	(terpri)
	(terpri)
	(prin1 g)
	(cond
		((null g)nil)
		(
			T
			(start (hide g (find_new_comp g (list(caar g))) nil))
		)
	)
)

;Поиск новой компоненты связности
;Накапливаем пул в v
(defun find_new_comp(g v) (add_v g v nil))

;Добавление новой вершины в компоненту
(defun add_v(g v s)
	(cond
		((null g)(double_print v s))
		(
			(or
				(and (member (caar g) v) (member (cadar g) v))
				(and (null(member (caar g) v)) (null(member (cadar g) v)))
			)
			(add_v (cdr g) v s)
		)
		(
			T 
			(cond 
				((member (caar g) v)(add_v (cdr g) (append v (cdar g)) (append s (list(car g))))) ;Прихватываем 2 
				(T (add_v (cdr g) (append v (list (caar g))) (append s (list(car g)))))			  ;1
			)
		)
	)
)

;Убираем просмотренное в temp_g - для деления на части
(defun hide(g v temp_g)
	(cond
		(
			(null g)
			(check_dichotomy temp_g)
		)
		(
			(or(member (caar g) v)(member (cadar g) v))
			(hide (cdr g) v (cons (car g) temp_g))
		)
		(T(cons (car g) (hide (cdr g) v temp_g))) ;А вот лишнее не хайдим!
	)
)

(defun double_print(v s)
	(terpri)
	(princ "Каркас: ")
	(prin1 s)
	(terpri)
	(princ "Состав вершин: ")
	(prin1 v)
)

;Проверка на двудольность
(defun check_dichotomy(g) 
	(check_euler g)
	(check_full g (cut_it g (list (caar g))nil))
)

;Разбиение вершин по двум долям
;Пришло время понять, кто "правый", а кто "левый"
(defun cut_it(g l r) (choose_way g l r))

;Добавление новой вершины к правым или левым
(defun choose_way(g l r)
	(cond
		((null g)(list l r))
		(
			(member (caar g) l)
			(cond 
				((member (cadar g) l)nil) ;л+л = плохо
				((member (cadar g) r)(choose_way (cdr g) l r)) ;ок - дальше
				(T(choose_way g l (append r (list (cadar g)))))
			)
		)
		(
			(member (caar g) r)
			(cond 
				((member (cadar g) r)nil) ;п+п = плохо
				((member (cadar g) l)(choose_way (cdr g) l r)) ;ок - дальше
				(T(choose_way g (append l (list (cadar g))) r))
			)
		)
		;Зацеп по второму "партнёру"
		(
			(member (cadar g) l)
			(cond 
				((member (caar g) l)nil)
				((member (caar g) r)(choose_way (cdr g) l r))
				(T(choose_way g l (append r (list (caar g)))))
			)
		)
		(
			(member (cadar g) r)
			(cond 
				((member (caar g) r)nil)
				((member (caar g) l)(choose_way (cdr g) l r))
				(T(choose_way g (append l (list (caar g))) r))
			)
		)
		(T(choose_way (cdr g) l r))
	)
)

(defun check_full (g c_g)
	(cond
		(
			(eq c_g nil)
			(terpri)
			(prin1 "Не двудольный")
			(terpri)
		)
		(
			(eq ;поиграем в считалочку
				(rel_count g (flatten_book c_g))
				(*(length(car c_g))(length(cadr c_g)))
			)
			(print "100% Двудольный")
			(terpri)
		)
		(
			T 
			(terpri)
			(print "Двудольный")
			(terpri)
		)
	)
)

;Подсчёт числа связей (relations)
;sp_g = отфлатанный список отчузанных || euler: = (pointer)
(defun rel_count(g sp_g &optional (c 0)) 
	(cond
		((null g) c)
		(
			(or(member(caar g)sp_g)(member(cadar g)sp_g))
			(rel_count (cdr g) sp_g (+ c 1))
		)
		(T(rel_count (cdr g) sp_g c))
	)
)

;Проверка на эйлеровость
(defun check_euler(g)
	(cond
		(
			(not(check_chet(flatten_book g)))
			(print "Не Эйлеров ")
			(terpri)
		)
		(T
			(print "Эйлеров цикл: ")
			(create_euler g (caar g) nil)
		)
	)
)

;Проверка меток на чётность
;(Передаём отфлатанный список g)
(defun check_chet(g_f)
	(cond
		((null g_f)T)
		((oddp (lable_count g_f (car g_f)))nil) ;нечёт
		(T(check_chet (create_shorter g_f (car g_f)))) 
	)
)

;Кол-во вхождений метки
(defun lable_count(g_f l &optional (c 0))
	(cond
		((null g_f)c)
		((eq l (car g_f))(lable_count (cdr g_f) l (+ c 1)))
		(T(lable_count (cdr g_f) l c))
	)
)

;Убираем проверенную метку из отфлатанного списка, прошедшую чётность
(defun create_shorter(g_f l)
	(cond
		((null g_f)nil) ;Всё опустошилось: nil - превратится в T
		((eq l (car g_f))(create_shorter (cdr g_f) l))
		(T(cons (car g_f) (create_shorter (cdr g_f) l)))
	)
)

;Построение эйлерова цикла
(defun create_euler(g pointer stack)
	(cond
		((eq 0 (rel_count g (list pointer)))(pop_stack g pointer stack)) ;начинаем попать - связей не осталось
		(
			T
			(add_stack g pointer stack)
		)
	)
)

;Эйлеров цикл: добавление/удаление рёбер
;"Берёмся за веревку и идём от узла к узлу"
(defun add_stack(g pointer stack)
	(cond
		(
			(or
				(eq (caar g)  pointer)
				(eq (cadar g) pointer)
			)
			(cond
				(
					(eq (caar g)  pointer) 
					(create_euler (cdr g) (cadar g) (cons (car g) stack))
				)
				(T
					(create_euler (cdr g) (caar g) (cons (car g) stack))
				)
			)
		)
		;указатель не "дорожке"
		(T
			(cond
				(
					(eq (caar g) pointer) ;цепочка идёт нормально
					(prin1 (car g)) ;На выход с вещами!
				)
				(T
					(prin1 (reverse(car g))) ;нет? ну тогда вертанём
				)
			)
			(create_euler (cdr g) pointer stack))
	)
)

;(defun helper(g pointer stack)
;	(create_euler g pointer stack)
;)

;Опустошаем трюмы
(defun pop_stack(g pointer stack)
	(cond
		((null stack) nil) ;Пусто - расходимся
		(T
			(cond
				(
					(eq (caar stack) pointer) ;цепочка идёт нормально
					(prin1 (car stack)) ;На выход с вещами!
					(create_euler g (cadar stack) (cdr stack)) 
				)
				(T
					(prin1 (reverse(car stack))) ;нет? ну тогда вертанём
					(create_euler g (caar stack) (cdr stack)) 
				)
			)
		)
	)
)

(defun flatten_book(x)
	(cond 
		((null x) NIL)
		((atom x) (cons x ()))
		(T (append (flatten_book (car x))(flatten_book (cdr x))))
	)
)

;(print (start '((1 2)(1 3)(3 5)(5 2))))
;(print (start '((1 2)(2 4)(4 5)(5 3)(3 1)(10 11)(11 12))))
;(print (start '((0 1)(2 4))))
;(print (start '((0 1)(1 2)(2 4))))
;(print (start '((4 5)(6 3)(5 1)(1 6))))
;(print (start '((1 2)(2 3)(3 4)(4 1))))

;(print (start '((1 3)(1 4)(2 4)(2 5)(3 5)(3 6)(3 7)(4 6)(4 7))))
;(print (start '((8 10)(8 11)(10 9)(9 11))))
;(print (start '((1 1)(2 2)(1 2))))
;(print (start '((3 4)(4 5)(5 7)(3 7))))
;(print (start '((4 5)(4 5)(4 5))))