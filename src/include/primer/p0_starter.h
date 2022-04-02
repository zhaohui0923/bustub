//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// p0_starter.h
//
// Identification: src/include/primer/p0_starter.h
//
// Copyright (c) 2015-2020, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <memory>
#include <stdexcept>
#include <vector>

#include "common/exception.h"

namespace bustub {

/**
 * The Matrix type defines a common
 * interface for matrix operations.
 */
template <typename T>
class Matrix {
 protected:
  /**
   * Construct a new Matrix instance.
   * @param rows The number of rows
   * @param cols The number of columns
   *
   */
  Matrix(int rows, int cols) : rows_(rows), cols_(cols) { linear_ = static_cast<T *>(malloc(sizeof(T) * rows * cols)); }

  /** The number of rows in the matrix */
  int rows_;
  /** The number of columns in the matrix */
  int cols_;

  /**
   * A flattened array containing the elements of the matrix.
   */
  T *linear_;

 public:
  /** @return The number of rows in the matrix */
  virtual int GetRowCount() const = 0;

  /** @return The number of columns in the matrix */
  virtual int GetColumnCount() const = 0;

  /**
   * Get the (i,j)th matrix element.
   *
   * Throw OUT_OF_RANGE if either index is out of range.
   *
   * @param i The row index
   * @param j The column index
   * @return The (i,j)th matrix element
   * @throws OUT_OF_RANGE if either index is out of range
   */
  virtual T GetElement(int i, int j) const = 0;

  /**
   * Set the (i,j)th matrix element.
   *
   * Throw OUT_OF_RANGE if either index is out of range.
   *
   * @param i The row index
   * @param j The column index
   * @param val The value to insert
   * @throws OUT_OF_RANGE if either index is out of range
   */
  virtual void SetElement(int i, int j, T val) = 0;

  /**
   * Fill the elements of the matrix from `source`.
   *
   * Throw OUT_OF_RANGE in the event that `source`
   * does not contain the required number of elements.
   *
   * @param source The source container
   * @throws OUT_OF_RANGE if `source` is incorrect size
   */
  virtual void FillFrom(const std::vector<T> &source) = 0;

  /**
   * Destroy a matrix instance.
   */
  virtual ~Matrix() { free(linear_); }
};

/**
 * The RowMatrix type is a concrete matrix implementation.
 * It implements the interface defined by the Matrix type.
 */
template <typename T>
class RowMatrix : public Matrix<T> {
 public:
  /**
   * Construct a new RowMatrix instance.
   * @param rows The number of rows
   * @param cols The number of columns
   */
  RowMatrix(int rows, int cols) : Matrix<T>(rows, cols) {
    data_ = static_cast<T **>(malloc(sizeof(T *) * rows));
    for (int row = 0; row < rows; ++row) {
      data_[row] = this->linear_ + row * cols;
    }
  }

  /**
   * @return The number of rows in the matrix
   */
  int GetRowCount() const override { return this->rows_; }

  /**
   * @return The number of columns in the matrix
   */
  int GetColumnCount() const override { return this->cols_; }

  /**
   * Get the (i,j)th matrix element.
   *
   * Throw OUT_OF_RANGE if either index is out of range.
   *
   * @param i The row index
   * @param j The column index
   * @return The (i,j)th matrix element
   * @throws OUT_OF_RANGE if either index is out of range
   */
  T GetElement(int i, int j) const override {
    if (i < 0 || i >= this->rows_ || j < 0 || j >= this->cols_) {
      throw Exception{ExceptionType::OUT_OF_RANGE, "RowMatrix::GetElement() OUT_OF_RANGE."};
    }
    return data_[i][j];
  }

  /**
   * Set the (i,j)th matrix element.
   *
   * Throw OUT_OF_RANGE if either index is out of range.
   *
   * @param i The row index
   * @param j The column index
   * @param val The value to insert
   * @throws OUT_OF_RANGE if either index is out of range
   */
  void SetElement(int i, int j, T val) override {
    if (i < 0 || i >= this->rows_ || j < 0 || j >= this->cols_) {
      throw Exception{ExceptionType::OUT_OF_RANGE, "RowMatrix::SetElement() OUT_OF_RANGE."};
    }
    data_[i][j] = val;
  }

  /**
   * Fill the elements of the matrix from `source`.
   *
   * Throw OUT_OF_RANGE in the event that `source`
   * does not contain the required number of elements.
   *
   * @param source The source container
   * @throws OUT_OF_RANGE if `source` is incorrect size
   */
  void FillFrom(const std::vector<T> &source) override {
    if (source.size() != static_cast<unsigned>(this->rows_) * static_cast<unsigned>(this->cols_)) {
      throw Exception{ExceptionType::OUT_OF_RANGE, "FillFrom() OUT_OF_RANGE."};
    }
    for (size_t i = 0; i < source.size(); ++i) {
      this->linear_[i] = source[i];
    }
  }

  /**
   * Destroy a RowMatrix instance.
   */
  ~RowMatrix() override { free(data_); }

 private:
  /**
   * A 2D array containing the elements of the matrix in row-major format.
   */
  T **data_;
};

/**
 * The RowMatrixOperations class defines operations
 * that may be performed on instances of `RowMatrix`.
 */
template <typename T>
class RowMatrixOperations {
 public:
  /**
   * Compute (`matrixA` + `matrixB`) and return the result.
   * Return `nullptr` if dimensions mismatch for input matrices.
   * @param matrixA Input matrix
   * @param matrixB Input matrix
   * @return The result of matrix addition
   */
  static std::unique_ptr<RowMatrix<T>> Add(const RowMatrix<T> *matrixA, const RowMatrix<T> *matrixB) {
    auto a_row_count = matrixA->GetRowCount();
    auto b_row_count = matrixB->GetRowCount();
    auto a_col_count = matrixA->GetColumnCount();
    auto b_col_count = matrixB->GetColumnCount();
    if (a_row_count != b_row_count || a_col_count != b_col_count) {
      return std::unique_ptr<RowMatrix<T>>(nullptr);
    }
    auto result = std::make_unique<RowMatrix<T>>(a_row_count, a_col_count);
    for (int row = 0; row < a_row_count; ++row) {
      for (int col = 0; col < a_col_count; ++col) {
        result->SetElement(row, col, matrixA->GetElement(row, col) + matrixB->GetElement(row, col));
      }
    }
    return result;
  }

  /**
   * Compute the matrix multiplication (`matrixA` * `matrixB` and return the result.
   * Return `nullptr` if dimensions mismatch for input matrices.
   * @param matrixA Input matrix
   * @param matrixB Input matrix
   * @return The result of matrix multiplication
   */
  static std::unique_ptr<RowMatrix<T>> Multiply(const RowMatrix<T> *matrixA, const RowMatrix<T> *matrixB) {
    auto a_row_count = matrixA->GetRowCount();
    auto b_row_count = matrixB->GetRowCount();
    auto a_col_count = matrixA->GetColumnCount();
    auto b_col_count = matrixB->GetColumnCount();
    if (a_row_count != b_row_count || a_col_count != b_col_count) {
      return std::unique_ptr<RowMatrix<T>>(nullptr);
    }
    auto result = std::make_unique<RowMatrix<T>>(a_row_count, a_col_count);
    for (int row = 0; row < a_row_count; ++row) {
      for (int col = 0; col < a_col_count; ++col) {
        result->SetElement(row, col, matrixA->GetElement(row, col) * matrixB->GetElement(row, col));
      }
    }
    return result;
  }

  /**
   * Simplified General Matrix Multiply operation. Compute (`matrixA` * `matrixB` + `matrixC`).
   * Return `nullptr` if dimensions mismatch for input matrices.
   * @param matrixA Input matrix
   * @param matrixB Input matrix
   * @param matrixC Input matrix
   * @return The result of general matrix multiply
   */
  static std::unique_ptr<RowMatrix<T>> GEMM(const RowMatrix<T> *matrixA, const RowMatrix<T> *matrixB,
                                            const RowMatrix<T> *matrixC) {
    auto a_row_count = matrixA->GetRowCount();
    auto b_row_count = matrixB->GetRowCount();
    auto c_row_count = matrixC->GetRowCount();
    auto a_col_count = matrixA->GetColumnCount();
    auto b_col_count = matrixB->GetColumnCount();
    auto c_col_count = matrixC->GetColumnCount();
    if (a_row_count != b_row_count || a_col_count != b_col_count || a_row_count != c_row_count ||
        a_col_count != c_col_count) {
      return std::unique_ptr<RowMatrix<T>>(nullptr);
    }
    auto result = std::make_unique<RowMatrix<T>>(a_row_count, a_col_count);
    for (int row = 0; row < a_row_count; ++row) {
      for (int col = 0; col < a_col_count; ++col) {
        result->SetElement(
            row, col, matrixA->GetElement(row, col) * matrixB->GetElement(row, col) + matrixC->GetElement(row, col));
      }
    }
    return result;
  }
};
}  // namespace bustub
