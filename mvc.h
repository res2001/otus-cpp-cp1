#pragma once

#include "base.h"
#include <memory>

/**
 * @class Rectangle
 * @brief Класс графического примитива "Прямоугольник".
 */
class Rectangle : public Shape {
private:
    Point m_topLeft; /**< Координата левого верхнего угла */
    int m_width;     /**< Ширина */
    int m_height;    /**< Высота */
public:
    Rectangle(Point topLeft, int width, int height, Color color) 
    : Shape(color), m_topLeft(topLeft), m_width(width), m_height(height) {}

    void Draw(IOutputContext& ctx) const override { ctx.drawRect(m_topLeft, m_width, m_height, m_color); }
};

/**
 * @class Circle
 * @brief Класс графического примитива "Круг".
 */
class Circle : public Shape {
private:
    Point m_center; /**< Центр круга */
    int m_radius;   /**< Радиус */
public:
    Circle(Point center, int radius, Color color) 
        : Shape(color), m_center(center), m_radius(radius) {}

    void Draw(IOutputContext& ctx) const override { ctx.drawCircle(m_center, m_radius, m_color); }
};
/**
 * @class Triangle
 * @brief Класс графического примитива "Треугольник".
 */
class Triangle : public Shape {
private:
    Point m_p1; /**< Первая вершина */
    Point m_p2; /**< Вторая вершина */
    Point m_p3; /**< Третья вершина */
public:
    Triangle(Point p1, Point p2, Point p3, Color color) 
        : Shape(color), m_p1(p1), m_p2(p2), m_p3(p3) {}

    void Draw(IOutputContext& ctx) const override { ctx.drawTriangle(m_p1, m_p2, m_p3, m_color); }
};

/**
 * @class ShapeFactory
 * @brief Фабрика для инкапсуляции процесса создания объектов графических фигур.
 */
struct ShapeFactory {
    /**
     * @brief Универсальный шаблонный метод для создания фигур.
     * @tparam Type Тип создаваемой фигуры из перечисления ShapeType.
     * @param args Вариативные аргументы, специфичные для конструктора конкретной фигуры.
     * @return std::unique_ptr<Shape> Умный указатель на созданную фигуру.
     */
    template <ShapeType Type, typename... Args>
    static std::unique_ptr<Shape> createShape(Args&&... args) {
        if constexpr (Type == ShapeType::Rectangle) {
            return std::make_unique<Rectangle>(std::forward<Args>(args)...);
        }
        else if constexpr (Type == ShapeType::Circle) {
            return std::make_unique<Circle>(std::forward<Args>(args)...);
        }
        else if constexpr (Type == ShapeType::Triangle) {
            return std::make_unique<Triangle>(std::forward<Args>(args)...);
        }
        else {
            return nullptr;
        }
    }
};
