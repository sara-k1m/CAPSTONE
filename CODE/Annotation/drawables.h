#ifndef EYEDID_CPP_SAMPLE_DRAWABLES_H_
#define EYEDID_CPP_SAMPLE_DRAWABLES_H_

#include <algorithm>
#include <mutex>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

#include "opencv2/opencv.hpp"

/**
 * OpenCV�� ����� UI ��Ҹ� �׸��� Ŭ����
 *
 * - �� ���(Circle, Text, Image)�� draw �޼��带 ���� ȭ�鿡 �׷���
 * - `draw` �޼���� ��Ұ� ���̴���(visibility)�� Ȯ������ ����
 * - ���ü��� Ȯ���Ϸ��� `draw_if`�� ���
 */

namespace sample {
namespace drawables {
// ��� �׸��� ����� �⺻ Ŭ����
struct DrawableBase {
bool visible = true; // ����� ���� ���θ� ��Ÿ�� (�⺻��: ����)
};

// ���� �׸��� ���� ����ü
struct Circle : protected DrawableBase {
  using DrawableBase::visible; // �θ� Ŭ������ visible ���� ���

  // ���� ȭ�鿡 �׸��� �Լ�
  void draw(cv::Mat* dst) const {
    cv::circle(*dst, center, radius, color, thickness, line_type, shift);
  }

  cv::Point center; // ���� �߽� ��ǥ
  int radius = 10; // ���� ������ (�⺻��: 10)
  cv::Scalar color; // ���� ���� (BGR ����)
  int thickness = -1; // �� �β� (-1�̸� ���θ� ä��)
  int line_type = cv::LINE_8; // �� ���� (�⺻��: LINE_8)
  int shift = 0; // �Ҽ��� ��ǥ ���е�
};

// �̹����� �׸��� ���� ����ü
struct Image : protected DrawableBase {
  using DrawableBase::visible; // �θ� Ŭ������ visible ���� ���

  // �̹����� ȭ�鿡 �׸��� �Լ�
  void draw(cv::Mat* dst) const {
    if (buffer.empty()) return; // �̹��� �����Ͱ� ������ �׸��� ����

    cv::resize(buffer, resized_, size); // �̹����� ���ϴ� ũ��� ����
    const auto img_w = std::min(resized_.cols, dst->cols - tl.x); // ȭ�� �ʺ� ����
    const auto img_h = std::min(resized_.rows, dst->rows - tl.y); // ȭ�� ���̿� ����
    resized_(cv::Rect(0, 0, img_w, img_h)).copyTo((*dst)(cv::Rect(tl.x, tl.y, img_w, img_h))); // �̹����� ����
  }
  cv::Point tl; // �̹����� �׸� ��ġ (���� ���)
  cv::Size size = { 100, 100 }; // �̹��� ũ�� (�⺻��: 100x100)
  cv::Mat buffer; // �̹��� ������

  private:
    mutable cv::Mat resized_; // ũ�� ������ �̹����� ���� (mutable: const �޼��忡���� ���� ����)
};

// �ؽ�Ʈ�� �׸��� ���� ����ü
struct Text : protected DrawableBase {
  using DrawableBase::visible; // �θ� Ŭ������ visible ���� ���

  // �ؽ�Ʈ�� ȭ�鿡 �׸��� �Լ�
  void draw(cv::Mat* dst) const {
    cv::putText(*dst, text, org, font_face, fontScale, color, thickness, line_type, bottom_left_origin);
  }

  cv::Point org; // �ؽ�Ʈ ���� ��ǥ
  std::string text; // ǥ���� �ؽ�Ʈ
  int font_face = cv::FONT_HERSHEY_PLAIN; // ��Ʈ ����
  double fontScale = 1; // ��Ʈ ũ�� ����
  cv::Scalar color = { 255, 255, 255 }; // �ؽ�Ʈ ���� (�⺻��: ���)
  int thickness = 1; // �� �β�
  int line_type = cv::LINE_8; // �� ����
  bool bottom_left_origin = false; // ��ǥ ������ (false: ���� ��� ����)
};

// Ư�� ��ü�� Drawable���� Ȯ���ϱ� ���� ���ø�
template<typename...> using void_t = void;

template<typename T, typename = void>
struct is_drawable : std::false_type {}; // �⺻������ Drawable�� �ƴ�

template<typename T>
struct is_drawable<
  T,
  void_t<decltype(std::declval<typename std::add_const<T>::type>().draw(static_cast<cv::Mat*>(nullptr)))>
> : std::true_type {}; // draw �޼��尡 ������ Drawable�� ����

// ��Ұ� ���� ������ ��쿡�� �׸��� �Լ�
template<typename Drawable>
typename std::enable_if<is_drawable<Drawable>::value>::type
draw_if(const Drawable& drawable, cv::Mat* dst) {
    if (drawable.visible) // ��Ұ� ���̴� �������� Ȯ��
        drawable.draw(dst); // ��� �׸���
}

// No-own erasure
// Drawable Ŭ����: ��� UI ��Ҹ� �߻�ȭ
class Drawable {
  struct DrawableInterface {
    virtual void draw(cv::Mat* dst) const = 0; // ��Ҹ� �׸��� �������̽�
  };

  // ��ü���� Drawable ��ü�� �����ϴ� Ŭ����
  template<typename T>
  struct DrawableConcrete : DrawableInterface {
    explicit DrawableConcrete(T* t) : object_(t) {}

    void draw(cv::Mat* dst) const override {
      if (object_->visible) // ���� ������ ���
        object_->draw(dst); // ��� �׸���
    }

    T* object_; // ���� ��ü ������
  };

  std::shared_ptr<DrawableInterface> ptr_; // Drawable ��ü ������

  template<typename T>
  T* get_ptr() {
    return dynamic_cast<DrawableConcrete<T>*>(ptr_.get())->object_; // ���� ��ü�� ��ȯ
  }

 public:
  Drawable() = default;

  // Drawable ��ü ������
  template<typename T, typename std::enable_if<is_drawable<T>::value, int>::type = 0>
  explicit Drawable(T* object) : ptr_(std::make_shared<DrawableConcrete<T>>(object)) {}

  // ���� ��ü�� ��ȯ
  template<typename T> T* get_as() { return get_ptr<T>(); }
  template<typename T> const T* get_as() const { return get_ptr<T>(); }

  // ��Ҹ� ȭ�鿡 �׸���
  void draw(cv::Mat* dst) const {
    ptr_->draw(dst);
  }
};

} // namespace drawables
} // namespace sample

#endif // EYEDID_CPP_SAMPLE_DRAWABLES_H_
