template <typename T>
class Node
{
	T data;
	Node* next;
};


template <typename T>
class Stack
{
std::shared_ptr<Node<T>> head;

public:

void push(T data)
{
	Node<T> * newnode = new Node<T>;
	newnode->data = data;
	newnode->next = head;
	while(!std::atomic_compare_exchange_weak(head, newnode->next,newnode));
}

std::shared_ptr<T> pop()
{
	auto old_head = head;
	while (old_head && !std::atomic_compare_exchange_weak(head, old_head,old_head->next));
	return old_head ? old_head->data : std::shared_ptr<T>();
}

};
