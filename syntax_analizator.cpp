#include "syntax_analizator.h"

#include <string.h>
#include <iostream>
#include <fstream>
using namespace std;

string lex_to_string(type_of_lex);
Sin_analizator::Sin_analizator(const std::vector<Lex> &l, const std::string &f)
{
    lex_vector = l;
    filename = f;
}

void Sin_analizator::gl()
{
    if(lex_vector.empty()) //если что-то пошло не так или нет нормального конца
        throw "вектор лексем пустой";
    curr_lex = lex_vector.front(); //берем лексемы от начала
    lex_vector.erase(lex_vector.begin()); //этот элемент, начальный, удаляем сразу
}

void Sin_analizator::show() const
{
    Ident *id;
    for(auto& i : var_map)
    {
        id = i.second; //*id - указатель на текущий идентификатор
        switch (id->get_type()) //для каждого типа свои возвращаемые типы, т.е. свой вывод
        {
        case LEX_INTEGER:
            cout << "ТИП = " << lex_to_string(id->get_type()) << " ИМЯ = " << id->get_name() << endl;
            cout << static_cast<Ident_Int*>(id)->get_value() << endl; //используем прообразование типов, чтобы определить нужный геттер
            break;
        case LEX_FLOAT:
            cout << "ТИП = " << lex_to_string(id->get_type()) << " ИМЯ = " << id->get_name() << endl;
            cout << static_cast<Ident_Float*>(id)->get_value() << endl;
            break;
        case LEX_RATIONAL:
            cout << "ТИП = " << lex_to_string(id->get_type()) << " ИМЯ = " << id->get_name() << endl;
            static_cast<Ident_Rational*>(id)->get_value().show(); //пользуемся выводом класса нашего Rational_number
            cout << endl;
            break;
        case LEX_VECTOR:
            cout << "ТИП = " << lex_to_string(id->get_type()) << " ИМЯ = " << id->get_name() << endl;
            static_cast<Ident_Vector*>(id)->get_value().show(); //ан-но
            break;
        case LEX_MATRIX:
            cout << "ТИП = " << lex_to_string(id->get_type()) << " ИМЯ = " << id->get_name() << endl;
            static_cast<Ident_Matrix*>(id)->get_value().show(); //ан-но
            break;
        default:
            cout << "Не определили"; //маловероятно, но оставим
            break;
        }
    }
}
void Sin_analizator::throw_str(const std::string &str) const
{
    throw "(" + to_string(curr_lex.get_line()) + ":" + to_string(curr_lex.get_column()) + ") " + str;
}
bool Sin_analizator::isoper(type_of_lex type) const
{
    switch (type) {
    case LEX_STAR: //*
        return true;
    case LEX_SLASH: // '/'
        return true;
    case LEX_PLUS: //+
        return true;
    case LEX_MINUS: //-
        return true;
    case LEX_POWER: //^
        return true;
    case LEX_EQUAL: //=
        return true;
    default:
        return false;
    }
}
bool Sin_analizator::isvar_const(type_of_lex type) const
{
    switch (type) {
    case LEX_DIGIT:
        return true;
    case LEX_DOUBLE:
        return true;
    case LEX_VAR:
    {
        if(var_map.find(curr_lex.get_value()) != var_map.end())  //если нет такой переменной
        {
            return true;
        }
        else throw_str("переменная " + curr_lex.get_value() + " не найдена");
    }
    case LEX_STRING:
        return true;
    default:
        return false;
    }
}
bool Sin_analizator::isfunction(type_of_lex type) const
{
    switch (type) {
    case LEX_CANONICAL:
        return true;
    case LEX_ROTATE:
        return true;
    case LEX_PRINT:
        return true;
    case LEX_WRITE:
        return true;
    case LEX_ROW:
        return true;
    case LEX_COLUMN:
        return true;
    case LEX_READ:
        return true;
    default:
        return false;
    }
}
void Sin_analizator::info()
{
    while(curr_lex.get_type() == LEX_INFO)
    {
        gl();
        info();
        if(curr_lex.get_type() != LEX_OPEN_ROUND_BRACKET) //нельзя без аргументов
            throw_str("ожидалась '('");
        gl();
        info();
        if(curr_lex.get_type() != LEX_STRING) //обязательно должен быть вывод чего-то
            throw_str("ожидалась строковая константа");
        gl();
        info();
        if(curr_lex.get_type() != LEX_CLOSE_ROUND_BRACKET)
            throw_str("ожидалась ')'");
        gl();
    }

}
void Sin_analizator::unary_op()
{
    while(curr_lex.get_type() == LEX_PLUS || curr_lex.get_type() == LEX_MINUS) //в т.ч. возможно считать много раз ++|--
    {
        if(curr_lex.get_type() == LEX_PLUS)
        {
            curr_lex.set_type(LEX_UNARY_PLUS);
    
        }
        else
        {
            curr_lex.set_type(LEX_UNARY_MINUS);
    
        }
        gl();
        info();
    }
    info();

}
void Sin_analizator::function()
{
    info();
    if(!isfunction(curr_lex.get_type())) //если нет в списке такой функции
        throw_str("ожидалась функция");
    if(curr_lex.get_type() == LEX_WRITE ||
            curr_lex.get_type() == LEX_READ) //функции с аргументами из строки
    {
        gl();
        info();
        if(curr_lex.get_type() != LEX_OPEN_ROUND_BRACKET) //если аргумент не указан
            throw_str("ожидалась '('");
        gl();
        info();
        if(curr_lex.get_type() != LEX_STRING) //если неверный формат
            throw_str("ожидалась строковая константа");
        gl();
        info();
        if(curr_lex.get_type() != LEX_CLOSE_ROUND_BRACKET) //если аргумент не закрыли или их более одного
            throw_str("ожидалась ')'");
    }
    else if(curr_lex.get_type() == LEX_ROW || curr_lex.get_type() == LEX_COLUMN) //взятие строки или столбца
    {
        //Lex open_br;
        gl();
        info();
        if(curr_lex.get_type() != LEX_OPEN_ROUND_BRACKET)
            throw_str("ожидалась '('");
        //open_br = curr_lex;
        gl();
        info();
        brackets(); //допускаем вариант выражения в скобках
        if(curr_lex.get_type() != LEX_CLOSE_ROUND_BRACKET)
        {
            //curr_lex = open_br;
            throw_str("незакрытая скобочка '('");
        }
    }
    gl();
    info();
}
void Sin_analizator::operations_squareb() //аналогично с другими состояниями операций
{

    info();
    if(curr_lex.get_type() == LEX_COLON)
    {
        gl();
        function();
        operations_squareb();
    }
    else if(curr_lex.get_type() == LEX_CLOSE_SQUARE_BRACKET) //если операций не было
        return;
    else if(isoper(curr_lex.get_type())
            || curr_lex.get_type() == LEX_COMMA) //одиночные символы: операция или разделение аргументов
    {

        gl(); //эта лексема будет обработана в состоянии квадратных скобок
    } 
    else if(curr_lex.get_type() == LEX_OPEN_SQUARE_BRACKET) //вложенность скобок - по аналогии в состояние квадратных скобок
    {
        Lex open_b = curr_lex;

        gl();
        info();
        square_brackets();
        if(curr_lex.get_type() != LEX_CLOSE_SQUARE_BRACKET)
        {
            curr_lex = open_b;
            throw_str("незакрытая скобочка '['");
        }
        gl();
        info();
    }
    else throw_str("ожидался оператор");
}
void Sin_analizator::square_brackets() //результат выражения - доступ к элементам матрицы/вектора
{
    if(curr_lex.get_type() == LEX_CLOSE_SQUARE_BRACKET) //не допускаем вырожденных
        throw_str("пустые '[]' ");
    while(curr_lex.get_type() != LEX_CLOSE_SQUARE_BRACKET &&
          curr_lex.get_type() != LEX_FIN) //ожидаем конца скобок или проге (не ок)
    {
        info();
        unary_op(); 
        if(curr_lex.get_type() == LEX_OPEN_ROUND_BRACKET)
        {
            Lex open_b = curr_lex;
            gl();
            info();
            brackets(); //выражение в скобках - доступ к элементам матрицы/вектора
            if(curr_lex.get_type() != LEX_CLOSE_ROUND_BRACKET)
            {
                curr_lex = open_b;
                throw_str("незакрытая '('");
            }
    
            gl();
            info();
        }
        else
        {
            if(!isvar_const(curr_lex.get_type())) throw_str("ожидался идентификатор или константа");
    
            gl();
            info();
        }
        operations_squareb();
    }
}
void Sin_analizator::operations_roundb()
{
    info();
    if(curr_lex.get_type() == LEX_COLON)  //функции
    {
        gl();
        function();
        operations_roundb();
    }
   // else if(curr_lex.get_type() == LEX_CLOSE_ROUND_BRACKET) return;
    else if(isoper(curr_lex.get_type())) //считываем операцию, а после вернемся в состояние скобок
    {
        gl();
    }
    else if(curr_lex.get_type() == LEX_OPEN_SQUARE_BRACKET) //взятие элементов для результата выражения в скобках
    {
        Lex open_b = curr_lex;
        gl();
        info();
        square_brackets();
        if(curr_lex.get_type() != LEX_CLOSE_SQUARE_BRACKET)
        {
            curr_lex = open_b;
            throw_str("незакрытая '['");
        }
        gl();
        info();
    }
    else throw_str("ожидался оператор");
}
void Sin_analizator::brackets()
{
    if(curr_lex.get_type() == LEX_CLOSE_ROUND_BRACKET) //вырожденный вариант отрицаем
        throw_str("пустые скобочки '()'");
    //Lex open_br;
    while(curr_lex.get_type() != LEX_CLOSE_ROUND_BRACKET && curr_lex.get_type() != LEX_FIN) //т.е. идем до конца скобок, по которым вошли
    {
        info();
        unary_op();
        if(curr_lex.get_type() == LEX_OPEN_ROUND_BRACKET) //рекурсивно можем сколько угодно входить в скобки
        {
            //open_br = curr_lex;
            gl();
            brackets(); 
            if(curr_lex.get_type() != LEX_CLOSE_ROUND_BRACKET) //баланс скобок в итоге будет проверен
            {
                //curr_lex = open_br;
                throw_str("незакрытая '('");
            }
            gl();
            info();
        }
        else
        {
            if(!isvar_const(curr_lex.get_type())) throw_str("ожидался идентификатор или константа");
            gl();
        }
        if (curr_lex.get_type() == LEX_CLOSE_ROUND_BRACKET) continue;
        operations_roundb();
        if (curr_lex.get_type() == LEX_CLOSE_ROUND_BRACKET) throw_str("ожидался идентификатор или константа");
    }
}
void Sin_analizator::operations_process()
{
    info(); 
    if(curr_lex.get_type() == LEX_COLON) //применение к выражению функции
    {
        gl();
        function();
        operations_process(); //можно рекурсивно
    }
    else if(isoper(curr_lex.get_type()) || curr_lex.get_type() == LEX_SEMICOLON) //операция или конец строки
    {
        gl();
    }
    else if(curr_lex.get_type() == LEX_OPEN_SQUARE_BRACKET) //выражения по взятию элемента
    {
        Lex open_b = curr_lex;

        gl();
        info();
        square_brackets(); //выражения для аргументов обрабатываем здесь
        if(curr_lex.get_type() != LEX_CLOSE_SQUARE_BRACKET) //проверка окончания
        {
            curr_lex = open_b;
            throw_str("незакрытая '['");
        }
        gl();
        info();
        operations_process();
    }
    else throw_str("ожидался оператор");
}
void Sin_analizator::expression()
{
    while(curr_lex.get_type() != LEX_FIN && curr_lex.get_type() != LEX_SEMICOLON) //после P2 может сразу идти конец
    {
        //Lex open_br; //нужно для сохранения скобки '('
        info();
        if(curr_lex.get_type() == LEX_SEMICOLON) //пропуск пустых строк
        {
            gl();
        }
        else
        {
            unary_op(); //проверяем унарный плюс и минус 
            if(curr_lex.get_type() == LEX_OPEN_ROUND_BRACKET) //открытие
            {
        
                //open_br = curr_lex;
                gl();
                brackets(); //анализ того, что в скобках
                if(curr_lex.get_type() != LEX_CLOSE_ROUND_BRACKET) //проверяем закрытие этих скобок
                {
                    //curr_lex = open_br;
                    throw_str("незакрытая '('");
                }
                gl();
                info();
                operations_process(); //переходим к операциям после скобок
                if (curr_lex.get_type() == LEX_SEMICOLON) throw_str("ожидался идентификатор или константа");
            }
            else
            {
                if(!isvar_const(curr_lex.get_type())) //если не число, строка или переменная
                    throw_str("ожидался идентификатор или константа");
                gl();
                operations_process();
                if (curr_lex.get_type() == LEX_SEMICOLON) throw_str("ожидался идентификатор или константа"); 
            }
        }
    }
}
void Sin_analizator::process_second()
{
    while(curr_lex.get_type() != LEX_FIN) //после этой части сразу конец
    {
        while(curr_lex.get_type() == LEX_SEMICOLON) //пропускаем пустые строки
            gl();
        expression();
    }
}
void Sin_analizator::process_first()
{
    if(curr_lex.get_type() == LEX_FIN) //для вырожденного случая
        return;
    process_second();
}
void Sin_analizator:: constructor1()
{
    string str = ""; //для записи лексем тут
    unsigned int line = 0;
    unsigned int column = 0;
    type_of_lex type = LEX_NULL; //тип рассматриваемой лексемы
    while(true)
    {
        line = curr_lex.get_line(); //позиция
        column = curr_lex.get_column();
        type = LEX_NULL;
        str = "";
        if(curr_lex.get_type() == LEX_MINUS) //отрицательные числа
        {
            gl();
            switch (curr_lex.get_type()) //в зависимости от типа выбираем задание значений
            {
            case LEX_DOUBLE: //float < 0
                str += '-';
                str += curr_lex.get_value();
                type = LEX_FLOAT; 
                gl();
                break;
            case LEX_DIGIT: //integer/rational_number
                str = "-";
                str = curr_lex.get_value();
                type = LEX_DIGIT;
                gl();
                if(curr_lex.get_type() == LEX_SLASH) //если записано рациональное число
                {
                    str += '/';
                    gl();
                    if(curr_lex.get_type() == LEX_MINUS) //2 минуса возможны
                    {
                        str += '-';
                        gl();
                    }
                    if(curr_lex.get_type() != LEX_DIGIT) //можно только целые числа
                        throw_str("ожидалось целое число");
                    str += curr_lex.get_value();
                    type = LEX_RATIONAL;
                    gl();
                }
                break;
            default: 
                throw_str("ожидалось целое число или дробное");
                break;
            }
        }
        else
        {
            if(curr_lex.get_type() != LEX_DIGIT && curr_lex.get_type() != LEX_DOUBLE && curr_lex.get_type() != LEX_STRING) //тут все возможные типы параметров
                throw_str("ожидалось целое число или дробное");
            if(curr_lex.get_type() == LEX_DIGIT) //аналогично целое или дробное
            {
                str += curr_lex.get_value();
                type = LEX_DIGIT;
                gl();
                if(curr_lex.get_type() == LEX_SLASH)
                {
                    str += '/';
                    gl();
                    if(curr_lex.get_type() == LEX_MINUS)
                    {
                        str += '-';
                        gl();
                    }
                    if(curr_lex.get_type() != LEX_DIGIT) throw_str("ожидалось целое число");
                    type = LEX_RATIONAL;
                    str += curr_lex.get_value();
                    gl();
                }
            }
            else if(curr_lex.get_type() == LEX_DOUBLE) //анно float > 0
            {
                type = LEX_FLOAT;
                str += curr_lex.get_value();
                gl();
            }
            else if (curr_lex.get_type() == LEX_STRING) //строковая константа или имя файла
            {
                type = LEX_STRING;
                str += curr_lex.get_value();
                gl();
            }
        }
        dec_vector.emplace_back(line, column, type, str); //конструирование лексемы и запись в конец вектора
        if(curr_lex.get_type() == LEX_CLOSE_ROUND_BRACKET) //выход из цикла и обратно в declaration
            return;
        if(curr_lex.get_type() != LEX_COMMA) //если нет корректного разделения
            throw_str("ожидалось ',' или ')");
        gl(); //если есть еще параметры - остаемся в цикле
    }
}
void Sin_analizator::constructor()
{
    if(curr_lex.get_type() == LEX_CLOSE_ROUND_BRACKET) //может быть фиктивное задание значения
        return;
    constructor1();
    gl();
}

string Sin_analizator::get_dec_str() const
{
    string ex = "";
    for(const auto &i : dec_vector)
    {
        ex += lex_to_string(i.get_type());
        ex += ", ";
    }
    ex[ex.length() - 2] = ')';
    return ex;
}
void Sin_analizator::push_dec(string &name, unsigned int l, unsigned int c)
{
    Ident_Int *idInt = nullptr; //заранее готовим возможные идентификаторы
    Ident_Float *idFLoat = nullptr;
    Ident_Rational *idRational = nullptr;
    Ident_Vector *idVector = nullptr;
    Ident_Matrix *idMatrix = nullptr;

    switch (curr_type) { 
    case LEX_INTEGER:
        if(dec_vector.size() == 0)
            idInt = new Ident_Int(name, 0, l, c); //определяем нулем по умолчанию
        else
        {
            if(dec_vector[0].get_type() != LEX_DIGIT && dec_vector.size() != 1) //нельзя более одного параметра или нецелочисленный тип
                throw_str("нет такого конструктора для целого");
            idInt = new Ident_Int(name, stoi(dec_vector[0].get_value()), l, c);  //пользуемся аналогом atoi
        }
        var_map.insert(pair<string, Ident*>(name, idInt)); //сохранение результата в виде пары 
        break;
    case LEX_FLOAT: //аналогично с integer
        if(dec_vector.size() == 0)
            idFLoat = new Ident_Float(name, 0, l, c);
        else
        {
            if(dec_vector[0].get_type() != LEX_FLOAT || dec_vector.size() != 1)
                throw_str("нет такого конструктора для дробного числа" + get_dec_str());
            idFLoat = new Ident_Float(name, stod(dec_vector[0].get_value()), l, c);
        }
        var_map.insert(pair<string, Ident*>(name, idFLoat));
        break;
    case LEX_RATIONAL:
        switch (dec_vector.size()) //смотрим на кол-во аргументов
        {
        case 0: //дробь 0/1
            idRational = new Ident_Rational(name, 0, l, c);
            break;
        case 1: //дробь в строке или от целого числа
            if(dec_vector[0].get_type() == LEX_DIGIT)
                idRational = new Ident_Rational(name, Rational_number(dec_vector[0].get_value().c_str()), l, c); //вызов конструктора от строки char*
            else if(dec_vector[0].get_type() == LEX_STRING ||
                    dec_vector[0].get_type() == LEX_RATIONAL)
            {
                try
                {
                    idRational = new Ident_Rational(name, Rational_number(dec_vector[0].get_value().c_str()), l ,c); //аналогичная сборка дроби
                }
                catch(...) 
                {
                }
            }
            else
                throw_str("нет такого конструктора для рационального числа"); //если неправильный тип
            break;
        case 2: //2 целых числа или 2 строки с целыми числами
            if((dec_vector[0].get_type() == LEX_DIGIT && dec_vector[1].get_type() == LEX_DIGIT) ||(dec_vector[0].get_type() == LEX_STRING && dec_vector[1].get_type() == LEX_STRING))
            {
                try
                {
                    idRational = new Ident_Rational(name, Rational_number(dec_vector[0].get_value().c_str(),dec_vector[1].get_value().c_str()), l ,c); //в любом случае работаем со строками
                }
                catch(...)
                {
                }
            }
            else
                throw_str("нет такого конструктора для рационального числа");
            break;
        default:
            throw_str("нет такого конструктора для рационального числа"); //для 3+ аргументов нет конструкторов
            break;
        }
        var_map.insert(pair<string, Ident*>(name, idRational));
        break;
    case LEX_VECTOR:
        switch (dec_vector.size())
        {
        case 0:
            idVector = new Ident_Vector(name, Vector(0,0), l, c); //делаем пустой вектора
            break;
        case 1:
            if(dec_vector[0].get_type() == LEX_STRING) //чтение из файла
            {
                try
                {
                    idVector = new Ident_Vector(name, Vector(dec_vector[0].get_value().c_str()), l, c); //надо аналогично перевести в char*
                }
                catch(...)
                {
                }
            }
            else
                throw_str("нет такого конструктора для вектора");
            break;
        case 2:
            if((dec_vector[0].get_type() == LEX_DIGIT) && (dec_vector[1].get_type() == LEX_DIGIT)) //если введен тип и длина вектора
            {
                try
                {
                    idVector = new Ident_Vector(name, Vector(stoi(dec_vector[0].get_value()),stoi(dec_vector[1].get_value())), l, c); //конструктор вектора из единиц или нулей
                }
                catch(...)
                {
                }
            }
            else
                throw_str("нет такого конструктора для вектора");
            break;
        default:
            throw_str("нет такого конструктора для вектора");
            break;
        }
        var_map.insert(pair<string, Ident*>(name, idVector));
        break;
    case LEX_MATRIX:
        switch (dec_vector.size())
        {
        case 0:
            idMatrix = new Ident_Matrix(name, Matrix(0,0,0), l, c); //создаем пустую матрицу
            break;
        case 1:
            if(dec_vector[0].get_type() == LEX_DIGIT) //единичная матрица квадратная
                idMatrix = new Ident_Matrix(name, Matrix(stoi(dec_vector[0].get_value())), l, c);
            else if(dec_vector[0].get_type() == LEX_STRING)
            {
                try
                {
                    idMatrix = new Ident_Matrix(name, Matrix(dec_vector[0].get_value().c_str()), l, c); //матрица из файла
                }
                catch(...)
                {
                }
            }
            else
                throw_str("нет такого конструктора для матрицы");
            break;
        case 3:
            if((dec_vector[0].get_type() == LEX_DIGIT) && (dec_vector[1].get_type() == LEX_DIGIT) && (dec_vector[2].get_type() == LEX_DIGIT)) //матрица из единиц или нулей m на n
            {
                try
                {
                    idMatrix = new Ident_Matrix(name,
                    Matrix(stoi(dec_vector[0].get_value()), stoi(dec_vector[1].get_value()),stoi(dec_vector[2].get_value())), l, c); //числа из строки в целые
                }
                catch(...)
                {
                }
            }
            else
                throw_str("нет такого конструктора для матрицы");
            break;
        default:
            throw_str("нет такого конструктора для матрицы");
            break;
        }
        var_map.insert(pair<string, Ident*>(name, idMatrix)); 
        break;
    default:
        throw "Неопределенный тип (?)";
        break;
    }
    dec_vector.clear(); //больше эти данные не пригодятся
}

void Sin_analizator::declaration()
{
    string name = ""; //сюда пишем имя переменной, с которой работаем
    Ident *id = nullptr; //нужно для проверки наличия переменной
    unsigned int line = 0;
    unsigned int column = 0;
    while(true)
    {
        if(curr_lex.get_type() == LEX_SEMICOLON) //если прочли все переменные в этом блоке
        {
            gl();
            return;
        }
        else
        {
            if(curr_lex.get_type() != LEX_VAR) //чекаем на идентификатор = имя
                throw_str("ожидалось имя");
            line = curr_lex.get_line(); //записали начальные параметры текущей возможной переменной
            column = curr_lex.get_column();
            name = curr_lex.get_value(); 
            if(var_map.find(name) != var_map.end()) //если уже было объявлено ранее
            {
                id = var_map.find(name)->second; //берем существующий идентификатор
                throw_str(string("повторное объявление '") + name + "'");
            }
            gl();
            if(curr_lex.get_type() == LEX_OPEN_ROUND_BRACKET) //т.е. нужно задать значение
            {
                gl(); //взяли лексему со значением для конструктора
                constructor();
            }
            push_dec(name, line, column); //для текущего идентификатора создали идентификатор нашего класса в словаре
            if(curr_lex.get_type() != LEX_COMMA && curr_lex.get_type() != LEX_SEMICOLON) //если после имени некорректный разделитель
                throw_str("ожидалось ',' или ';'");
            if(curr_lex.get_type() == LEX_COMMA) //если есть еще переменные этого типа - след. итерация цикла
                gl();
        }
    }
}
void Sin_analizator::declare()
{
    while(true) //одна итерация цикла - группа переменых одного типа
    {
        while(curr_lex.get_type() == LEX_SEMICOLON) //пропускаем пустые объявления
            gl();
        curr_type = curr_lex.get_type(); //запоминаем, какого типа должны быть переменные после :
        if(curr_type != LEX_INTEGER && curr_type != LEX_FLOAT && curr_type != LEX_RATIONAL && curr_type != LEX_VECTOR && curr_type != LEX_MATRIX && curr_type != LEX_PROCESS) //проверяем, что объявлен один из корректных типов или начало выражений
            throw_str("ожидалось 'integer' или 'float' или 'rational' или 'vector'"
                     " или 'matrix' или'process");
        gl(); 
        if(curr_lex.get_type() != LEX_COLON) //после типа чекаем ':'
            throw_str("' ожидалась ':'");
        gl(); 
        if(curr_type == LEX_PROCESS) //если закончились объявления
        {
            process_first();
            return;
        }
        declaration(); //либо поочередно читаем переменые этого типа
    }
}

void Sin_analizator::scan()
{
    gl();
    if(curr_lex.get_type() == LEX_DECLARE) //если есть часть с declare:
    {
        gl();
        if(curr_lex.get_type() != LEX_COLON)
            throw_str("' ожидалась ':'");
        gl();
        declare();
    }
    else //если сразу пошли выражения
    {
        if(curr_lex.get_type() != LEX_PROCESS) //проверяем, что есть process
            throw_str("ожидалось 'process'");
        gl();
        if(curr_lex.get_type() != LEX_COLON) //и следующая лексема :
            throw_str("' ожидалась ':'");
        gl();
        process_first(); //обработка синтаксиса выражений
    }
}

Sin_analizator::~Sin_analizator()
{
    for(auto& i : var_map) //автоматический проход по контейнерам
        delete i.second;
}
