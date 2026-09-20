#pragma once

#include <cstddef>
#include <cstdint>
#include <array>
#include <string_view>

/**
 * @enum Color
 * @brief Доступные цвета для графических примитивов.
 */
enum class Color {
    Red= 0,
    Green,
    Blue,
    White,
    Black,
};

/**
 * @struct ColorMapEntry
 * @brief Элемент constexpr-словаря для сопоставления перечисления цвета с его текстовым именем.
 */
struct ColorMapEntry {
    Color color;             ///< Идентификатор цвета
    std::string_view name;   ///< Текстовое название цвета
};

/**
 * @class ColorRegistry
 * @brief Статический реестр для работы с цветами на этапе компиляции.
 */
class ColorRegistry {
private:
    static constexpr size_t m_colorCount = 5;
    static constexpr std::array<ColorMapEntry, m_colorCount> m_colorMap{{
        { Color::Red,   "Red"},
        { Color::Green, "Green"},
        { Color::Blue,  "Blue"},
        { Color::White, "White"},
        { Color::Black, "Black"}
    }};

public:
    /**
     * @brief Преобразует значение перечисления Color в строковое представление.
     * @param color Значение цвета из перечисления.
     * @return std::string_view Текстовое название цвета.
     */
    static constexpr std::string_view to_string(Color color) {
        for (const auto& it: m_colorMap) {
            if (it.color == color)
                return it.name;
        }
        return "Unknown";
    }

    /**
     * @brief Возвращает количество цветов в перечислении.
     */
    static constexpr size_t getColorCount() { return m_colorCount; }
};

/**
 * @struct Point
 * @brief Структура для представления координат двумерной точки на плоскости.
 */
struct Point {
    int x = 0;
    int y = 0;
};

/**
 * @class IOutputContext
 * @brief Абстрактный интерфейс графического контекста (холста) для отрисовки фигур.
 */
struct IOutputContext {
    virtual ~IOutputContext() = default;

    /**
     * @brief Вывод прямоугольника.
     * @param topLeft Точка левого верхнего угла.
     * @param w Ширина прямоугольника.
     * @param h Высота прямоугольника.
     * @param color Цвет заливки.
     */
    virtual void drawRect(  Point topLeft, int w, int h, Color color) = 0;

    /**
     * @brief Вывод круга.
     * @param center Точка центра круга.
     * @param r Радиус круга.
     * @param color Цвет заливки.
     */
    virtual void drawCircle(Point center, int r, Color color) = 0;

    /**
     * @brief Вывод треугольника по трем вершинам.
     * @param p1 Первая вершина.
     * @param p2 Вторая вершина.
     * @param p3 Третья вершина.
     * @param color Цвет заливки.
     */
    virtual void drawTriangle(Point p1, Point p2, Point p3, Color color) = 0;
};

/**
 * @enum ShapeType
 * @brief Перечисление поддерживаемых типов геометрических фигур.
 */
enum class ShapeType { 
    None = 0,
    Rectangle,
    Circle,
    Triangle,
    Max
};

/**
 * @class Shape
 * @brief Абстрактный базовый класс для всех графических примитивов.
 */
class Shape {
protected:
    Color m_color;  ///< Цвет фигуры

public:
    /**
     * @brief Конструктор базового класса фигуры.
     * @param color Цвет примитива.
     */
    Shape(Color color) :m_color(color) {}
    virtual ~Shape() = default;

    /**
     * @brief Чисто виртуальный метод для отрисовки фигуры на указанном контексте.
     * @param context Ссылка на графический контекст.
     */
    virtual void Draw(IOutputContext& context) const = 0;
};

/**
 * @class IObserver
 * @brief Интерфейс Наблюдателя паттерна Observer для обновления View при изменении Model.
 */
class IObserver {
public:
    virtual ~IObserver() = default;
    
    /**
     * @brief Коллбэк, вызываемый Моделью при изменении ее внутреннего состояния.
     */
    virtual void onModelChanged() = 0;
};
