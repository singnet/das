from queue import Queue


class SharedQueue:
    def __init__(self):
        self.queue = Queue()

    def enqueue(self, request):
        self.queue.put(request)

    def dequeue(self):
        if not self.empty():
            return self.queue.get()
        return None

    def empty(self) -> bool:
        return self.queue.empty()
