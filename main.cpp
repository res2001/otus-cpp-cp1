/**
 * @file main.cpp
 * @brief Макет графического редактора на чистом C++ с использованием паттерна MVC.
 */

#include "mvc.h"
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

/**
 * @class DocumentModel
 * @brief Модель (Model). Хранит имя документа и вектор примитивов сцены. Не зависит от View и Controller.
 */
class DocumentModel {
private:
    std::string m_documentName;                         ///< Имя текущего открытого документа
    std::vector<std::unique_ptr<Shape>> m_primitives;   ///< Контейнер, владеющий фигурами сцены
    std::vector<IObserver*> m_observers;                ///< Список зарегистрированных наблюдателей (представлений)

    /**
     * @brief Оповещает все зарегистрированные View об изменениях данных.
     */
    void notifyObservers() {
        for (auto* observer : m_observers) {
            if (observer) observer->onModelChanged();
        }
    }

public:
    /**
     * @brief Регистрирует новый объект View в качестве наблюдателя.
     * @param observer Указатель на объект интерфейса IObserver.
     */
    void addObserver(IObserver* observer) { m_observers.push_back(observer); }

    /**
     * @brief Метод бизнес-логики: Сброс сцены и создание нового документа.
     */
    void createNewDocument() {
        m_documentName = "Untitled.drw"; 
        m_primitives.clear();
        std::cout << "[Model] Данные сброшены.\n";
        notifyObservers();
    }

    /**
     * @brief Метод бизнес-логики: Загрузка (импорт) данных из файла на диске.
     * @param filename Путь к файлу.
     */
    void importFromFile(const std::string& filename) {
        m_documentName = filename; 
        m_primitives.clear();
        /* Эмитируем наличие в файле некоторых фигур */
        m_primitives.push_back(ShapeFactory::createShape<ShapeType::Circle>(Point{20, 20}, 10, Color::Red));
        m_primitives.push_back(ShapeFactory::createShape<ShapeType::Rectangle>(Point{0, 0}, 10, 10, Color::Blue));

        std::cout << "[Model] Импортирован файл: " << filename << "\n";
        notifyObservers();
    }

    /**
     * @brief Метод бизнес-логики: Сохранение (экспорт) данных в файл на диске.
     * @param filename Путь к файлу.
     */
    void exportToFile(const std::string& filename) {
        m_documentName = filename;
        /* Тут как-будто происходит экспорт в файл */
        std::cout << "[Model] Экспорт в файл: '" << m_documentName << "\n";
    }

    /**
     * @brief Метод бизнес-логики: Добавление графической фигуры на сцену.
     * @param shape Умный указатель std::unique_ptr с передачей владения фигурой.
     */
    void addPrimitive(std::unique_ptr<Shape> shape) {
        m_primitives.push_back(std::move(shape));
        notifyObservers();
    }
    
    /**
     * @brief Метод бизнес-логики: Удаление графической фигуры со сцены по индексу.* 
     * @param index Порядковый номер фигуры в векторе.
     */
    void deletePrimitive(size_t index) {
        if (index < m_primitives.size()) {
            m_primitives.erase(m_primitives.begin() + index);
            notifyObservers();
        } else {
            std::cerr << "[Model] Ошибка: Индекс (" << index << ") не существует.\n";
        }
    }

    std::string getDocumentName() { return m_documentName; }
    const std::string getDocumentName() const { return m_documentName; }
    std::vector<std::unique_ptr<Shape>>& getPrimitives() { return m_primitives; }
    const std::vector<std::unique_ptr<Shape>>& getPrimitives() const { return m_primitives; }
};

/**
 * @class DocumentController
 * @brief Контроллер (Controller). Принимает запросы от View, валидирует или преобразует их в вызовы бизнес-логики Модели.
 */
class DocumentController {
private:
    DocumentModel& m_model; ///< Ссылка на управляемую модель
public:
    /**@brief Конструктор контроллера.
     * @param model Ссылка на объект Модели.
     */
    explicit DocumentController(DocumentModel& model) : m_model(model) {}
    /// Обработчик запроса на создание документа
    void handleNewDocument() { m_model.createNewDocument(); }
    /// Обработчик запроса на импорт файла
    void handleImport(const std::string& path) { m_model.importFromFile(path); }
    /// Обработчик запроса на экспорт файла
    void handleExport(const std::string& path) { m_model.exportToFile(path); }
    /// Обработчик запроса на удаление фигуры
    void handleDeletePrimitive(size_t index) { m_model.deletePrimitive(index); }
    /**
     * @brief Шаблонный метод для добавления фигуры.
     * @tparam Type Тип добавляемой фигуры из перечисления ShapeType.
     * @param args Специфичные аргументы геометрии и цвета для фабрики.
     */
    template <ShapeType Type, typename... Args>
    void handleAddPrimitive(Args&&... args) {
        auto newShape = ShapeFactory::createShape<Type>(std::forward<Args>(args)...);
        if (newShape) {
            m_model.addPrimitive(std::move(newShape));
        }
    }
};

/**
 * @class ConsoleGraphicsContext
 * @brief Конкретная реализация графического контекста, имитирующая рендеринг через вывод текстовых сообщений.
 */
struct ConsoleContext : public IOutputContext {
public:
    void drawRect(const Point topLeft, int w, int h, Color color) override {
        std::cout << "[Рендер] Прямоугольник (Цвет: " << ColorRegistry::to_string(color) 
                  << ", Координаты левого верхнего угла: (" << topLeft.x << ", " << topLeft.y 
                  << "), Размер: " << w << "x" << h << ")";
    }
    void drawCircle(Point center, int r, Color color) override {
        std::cout << "[Рендер] Круг (Цвет: " << ColorRegistry::to_string(color) 
                  << ", Центр: (" << center.x << ", " << center.y 
                  << "), Радиус: " << r << ")";
    }
    void drawTriangle(Point p1, Point p2, Point p3, Color color) override {
        std::cout << "[Рендер] Треугольник (Цвет: " << ColorRegistry::to_string(color) 
                  << ", Вершины: P1(" << p1.x << ", " << p1.y << "), "
                  << "P2(" << p2.x << ", " << p2.y << "), "
                  << "P3(" << p3.x << ", " << p3.y << ")";
    }
};

/**
 * @class DocumentView
 * @brief Представление (View). Отвечает за рендеринг Модели и сбор пользовательского ввода (Интерактивное меню).
 */
class ConsoleView : public IObserver {
private:
    const DocumentModel& m_model;               ///< Ссылка на модель для чтения актуальных данных
    DocumentController& m_controller;           ///< Ссылка на контроллер для отправки пользовательских действий
    ConsoleContext m_context;                   ///< Контекст (холст) для отрисовки фигур

    std::string InputFileName(const std::string msg) {
        std::string filename;
        std::cout << msg << ": ";
        std::cin >> filename;
        return filename;
    }

    void InputPoint(Point& p, std::string msg) {
        std::cout << msg << ": ";
        std::cin >> p.x >> p.y;
    }

    void InputRectangle(Color color) {
        Point tl{};
        int width = 0, height = 0;
        InputPoint(tl, "Координаты левого верхнего угла прямоугольника x и y (через пробел)");
        std::cout << "Ширина: "; std::cin >> width;
        std::cout << "Высота: "; std::cin >> height;
        m_controller.handleAddPrimitive<ShapeType::Rectangle>(tl, width, height, color);
    }

    void InputTriangle(Color color) {
        std::array<Point, 3> p{};
        for (size_t i = 0; i < p.size(); ++i) {
            InputPoint(p[i], std::string("Координаты точки ") + std::to_string(i + 1) + "(через пробел)");
        }
        m_controller.handleAddPrimitive<ShapeType::Triangle>(p[0], p[1], p[2], color);
    }

    void InputCircle(Color color) {
        Point center{};
        int radius = 0;
        InputPoint(center, "Координаты центра окружности (через пробел): ");
        std::cout << "Радиус: "; std::cin >> radius;
        m_controller.handleAddPrimitive<ShapeType::Circle>(center, radius, color);
    }

public:
    /**
     * @brief Конструктор Представления.
     * @param model Ссылка на объект Модели.
     * @param controller Ссылка на объект Контроллера.
     */
    ConsoleView(const DocumentModel& model, DocumentController& controller)
    : m_model(model), m_controller(controller), m_context() {}
    /**
     * @brief Реализация интерфейса IObserver. Очищает и заново "отрисовывает" сцену в консоли при изменении данных.
     */
    void onModelChanged() override {
        std::cout << "\n================= [ConsoleView] =================\n";
        std::cout << "  Файл: " << m_model.getDocumentName() << "\n";
        std::cout << "  Элементы сцены:\n";
        const auto& primitives = m_model.getPrimitives();
        if (primitives.empty()) {
            std::cout << "    (пусто)\n";
        } else {
            for (size_t i = 0; i < primitives.size(); ++i) {
                std::cout << "    [" << i << "] ";
                primitives[i]->Draw(m_context);
                std::cout << "\n";
            }
        }
        std::cout << "============================================================\n\n";
    }
    /**
     * @brief Главный интерактивный текстовый цикл меню. Захватывает ввод пользователя.
     */
    void runMenuLoop() {
        int choice = 0;
        while (true) {
            std::cout << "--- МЕНЮ ---\n";
            std::cout << "1. Создать документ\n";
            std::cout << "2. Импортировать из файла\n";
            std::cout << "3. Экспортировать в файл\n";
            std::cout << "4. Добавить прямоугольник\n";
            std::cout << "5. Добавить окружность\n";
            std::cout << "6. Добавить треугольник\n";
            std::cout << "7. Удалить фигуру\n";
            std::cout << "0. Закрыть приложение\n";
            std::cout << "Выберите действие и нажмите Enter (0-7): ";

            if (!(std::cin >> choice)) {
                std::cin.clear(); 
                std::cin.ignore(10000, '\n');
                std::cout << "Ошибка ввода.\n\n";
                continue;
            }

            switch (choice)
            {
            case 0:
                return;

            case 1:
                m_controller.handleNewDocument();
                break;
            
            case 2:
                m_controller.handleImport(InputFileName("Имя файла для импорта"));
                break;
            case 3:
                m_controller.handleImport(InputFileName("Имя файла для экспорта"));
                break;

            case 4:
            case 5:
            case 6: {
                uint32_t color = 0;
                std::cout << "Введите параметры фигуры:\n";
                std::cout << "Цвет заливки (0-Красный, 1-Зеленый, 2-Синий, 3-Белый, 4-Черный): ";
                std::cin >> color;
                if (color >= ColorRegistry::getColorCount()) {
                    std::cout << "Неверный цвет. Установлен Черный по умолчанию.\n";
                    color = static_cast<int>(Color::Black);
                }
                switch(static_cast<ShapeType>(choice - 3))
                {
                case ShapeType::Rectangle:
                    InputRectangle(static_cast<Color>(color));
                    break;
                case ShapeType::Circle:
                    InputCircle(static_cast<Color>(color));
                    break;
                case ShapeType::Triangle:
                    InputTriangle(static_cast<Color>(color));
                    break;
                default:
                    std::cerr << "Unknown shape type: " << choice - 3 << "\n";
                    std::abort();
                }
                break;
            }

            case 7: {
                size_t index = 0;
                std::cout << "Индекс для удаления: "; std::cin >> index;
                m_controller.handleDeletePrimitive(index);
                break;
            }

            default:
                std::cerr << "Неверная команда.\n\n";
                break;
            }
        }
    }
};

int main() {
    DocumentModel model;
    DocumentController controller(model);
    ConsoleView view(model, controller);
    model.addObserver(& view);
    controller.handleNewDocument();
    view.runMenuLoop();
    return 0;
}