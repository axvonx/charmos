#include <block/bio.h>
#include <block/block.h>
#include <block/sched.h>
#include <structures/dll.h>
#include <structures/list.h>
#include <sync/mutex.h>
#include <types/types.h>

/* enqueuing skips enqueuing if the req is URGENT */

void bio_sched_enqueue_internal(struct bio_scheduler *sched,
                                struct bio_request *req) {
    if (submit_if_urgent(sched, req))
        return;

    MUTEX_ASSERT_HELD(&sched->lock);
    update_request_timestamp(req);
    enum bio_request_priority prio = req->priority;
    struct bio_rqueue *q = &sched->queues[prio];

    list_head_push_back(&q->list, &req->list);

    q->dirty = true;
    q->request_count++;
    sched->total_requests++;
}

void bio_sched_dequeue_internal(struct bio_scheduler *sched,
                                struct bio_request *req) {
    MUTEX_ASSERT_HELD(&sched->lock);
    enum bio_request_priority prio = req->priority;
    struct bio_rqueue *q = &sched->queues[prio];

    list_node_delete_init(&req->list);

    q->dirty = true;
    q->request_count--;
    sched->total_requests--;
}
