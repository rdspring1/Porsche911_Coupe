enum page_status
  {
    MEMORY,   /* Page currently stored in memory */
    DISK,     /* Page stored on the disk */
    SWAP,     /* Page swapped out of memory */
    SHARED    /* Page already located in physical memory */
  };

struct spage
{
    struct hash_elem hash_elem; /* Hash table element. */
    void *addr;                 /* Virtual address. */
    enum page_state;            /* Location of Page */
    void *shared;		           /* Memory Address of Shared Page */
    bool readonly;              /* Read Only Setting */
};

unsigned spage_hash (const struct hash_elem *p_, void *aux UNUSED);
bool spage_less (const struct hash_elem *a_, const struct hash_elem *b_, void *aux UNUSED);
struct spage * spage_lookup (struct spagedir pages, const void *address);
struct spage * spage_delete (struct spagedir pages, const void *address);
