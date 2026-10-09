# Systems Programming Memory Manager

## Overview

This project implements a custom memory manager in C using a bitmap-based allocation strategy.

The memory manager tracks allocated and free memory blocks using a bitmap and provides basic memory allocation and deallocation functionality. The project demonstrates low-level systems programming concepts such as pointer manipulation, memory layout, block tracking, and manual resource management.

## Features

- Custom memory allocation logic
- Bitmap-based tracking of used and free blocks
- Memory deallocation support
- Header file interface for memory manager functions
- Test program for validating allocation behavior
- Written in C for systems programming practice

## Project Structure

```text
systems-programming-memory-manager/
├── bitmap.c
├── common.h
├── memory_manager.c
├── memory_manager.h
├── test_main.c
└── test
