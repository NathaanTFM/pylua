from typing import Any, Callable
from typing_extensions import Protocol

_LuaObj = LuaObject | str | int | float | None

class _LuaCallable(Protocol):
    def __call__(self, *args: _LuaObj) -> _LuaObj | tuple[_LuaObj, ...]:
        ...

class LuaError(Exception):
    ...

class LuaCompileError(LuaError):
    ...

class LuaRuntimeError(LuaError):
    ...

class LuaFatalError(LuaError):
    ...

class LuaObject:
    ...

class LuaUserData(LuaObject):
    ...

class LuaThread(LuaObject):
    def call(self, func: LuaObject, *args: _LuaObj) -> tuple[_LuaObj, ...]:
        ...
    
    def get_state(self) -> LuaState:
        ...

class LuaFunction(LuaObject):
    def __call__(self, *args: _LuaObj) -> tuple[_LuaObj, ...]:
        ...

class LuaTable(LuaObject):
    def __getattr__(self, name: str) -> _LuaObj:
        ...
        
    def __setattr__(self, name: str, value: _LuaObj) -> None:
        ...

    def __getitem__(self, key: _LuaObj) -> _LuaObj:
        ...

    def __setitem__(self, key: _LuaObj, value: _LuaObj) -> None:
        ...


class LuaState:
    mem_limit: int
    time_limit: int

    def __init__(self, /, openlibs: int = 1) -> None:
        ...

    @property
    def mem_usage(self) -> int:
        ...

    def get_globals(self) -> LuaTable:
        ...

    def load_string(self, /, script: str, name: str = None, mode: str = None) -> LuaFunction:
        ...

    def load_file(self, /, filename: str, mode: str = None) -> LuaFunction:
        ...

    def new_table(self) -> LuaTable:
        ...

    def new_userdata(self, /, pyobj: Any) -> LuaUserData:
        ...

    def new_function(self, /, func: _LuaCallable) -> LuaFunction:
        ...

    def close(self) -> bool:
        ...