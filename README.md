Pole
portable library for structured storage

POLE is a portable C++ library to access structured storage. It is designed to be compatible with Microsoft structured storage, also sometimes known as OLE Compound Document.

A structured storage is normally used to store files inside another file; you can even have complex directory tree inside. It can be treated as a filesystem inside a file. The most popular use of structured storage is in Microsoft Office. Structured storage does not offer compression nor encryption support, hence usually it is used only for interoperability purposes.

Compared to structured storage routines offered by Microsoft Windows API, POLE has the advantage that it is portable. You can access any structured storage in the supported platform, you don't need to rely on Windows library.

This library is initially developed by Ariya Hidayat <ariya@kde.org>