/**
 * @file stats.h
 * @brief Spinlock-guarded statistics increment/reset macros for module stats_t.
 *
 * Requires stats struct fields: error_count (uint32_t), last_error_code (int32_t)
 * for STATS_ERROR. STATS_RESET is for module init only — not concurrent-safe.
 *
 * Hot paths: one logical event (callback exit, enqueue drop, etc.) should batch
 * related field updates under a single lock via STATS_SCOPE. Single-field cold
 * paths use STATS_INC / STATS_ERROR. Getters copy out via STATS_COPY_OUT.
 *
 * @see README.md for full context.
 */

#ifndef SYSTEM_STATS_H
#define SYSTEM_STATS_H

#include <string.h>
#include <zephyr/spinlock.h>

/**
 * @brief Increment a stats field by 1.
 * @isr_safe
 */
#define STATS_INC(lock, stats, field)               \
    do {                                            \
        k_spinlock_key_t _key = k_spin_lock(lock);  \
        (stats).field++;                            \
        k_spin_unlock(lock, _key);                  \
    } while (0)

/**
 * @brief Increment a stats field by @p val.
 * @isr_safe
 */
#define STATS_ADD(lock, stats, field, val)          \
    do {                                            \
        k_spinlock_key_t _key = k_spin_lock(lock);  \
        (stats).field += (val);                     \
        k_spin_unlock(lock, _key);                  \
    } while (0)

/**
 * @brief Decrement a stats field by 1. Use for gauges (e.g. inflight_now).
 * @isr_safe
 */
#define STATS_DEC(lock, stats, field)               \
    do {                                            \
        k_spinlock_key_t _key = k_spin_lock(lock);  \
        (stats).field--;                            \
        k_spin_unlock(lock, _key);                  \
    } while (0)

/**
 * @brief Record an error: increments error_count and stores errno.
 * @isr_safe
 */
#define STATS_ERROR(lock, stats, code)              \
    do {                                            \
        k_spinlock_key_t _key = k_spin_lock(lock);  \
        (stats).error_count++;                      \
        (stats).last_error_code = (int32_t)(code);  \
        k_spin_unlock(lock, _key);                  \
    } while (0)

/**
 * @brief Batch multiple field updates under a single lock acquisition.
 *
 * Use for hot paths where one logical event updates more than one field.
 * For single-field updates on cold paths, prefer STATS_INC / STATS_ERROR.
 *
 * Compute timestamps and other inputs before entering the scope — do not
 * call timing helpers inside the lock.
 *
 * @isr_safe
 *
 * Example:
 * @code
 *   uint32_t elapsed_us = k_cyc_to_us_floor32(k_cycle_get_32() - t0);
 *   STATS_SCOPE(&s_stats_lock, {
 *       s_stats.packets_received++;
 *       s_stats.packets_dropped_by_filter[stage]++;
 *       latency_stats_record(&s_stats.callback_time_us, elapsed_us);
 *   });
 * @endcode
 */
#define STATS_SCOPE(lock, body)                         \
    do {                                                \
        k_spinlock_key_t _key = k_spin_lock(lock);      \
        body;                                           \
        k_spin_unlock(lock, _key);                      \
    } while (0)

/**
 * @brief Reset all stats fields to zero (memset).
 *
 * Not concurrent-safe on its own — a concurrent STATS_INC (single-word
 * write under spinlock) races against the multi-word memset.
 *
 * Rules (XC-7):
 *   - From module init (before any concurrent writer exists): call freely.
 *   - From any *_reset_stats() function callable at runtime (e.g. shell
 *     commands): MUST acquire the module's stats spinlock before calling
 *     this macro, then release it after. Example:
 *
 *       k_spinlock_key_t key = k_spin_lock(&s_stats_lock);
 *       STATS_RESET(s_stats);
 *       k_spin_unlock(&s_stats_lock, key);
 *
 * @caller_sync when called from init; hold stats spinlock when called
 * from a live reset path (shell command, runtime reset API).
 */
#define STATS_RESET(stats) memset(&(stats), 0, sizeof(stats))

/**
 * @brief Copy stats struct under spinlock into caller buffer.
 * @isr_safe
 *
 * @param lock  Module stats spinlock (e.g. &s_stats_lock).
 * @param src   Pointer to module stats instance (e.g. &s_stats).
 * @param dst   Caller-owned destination buffer; NULL → -EINVAL.
 * @param sz    sizeof the stats struct.
 *
 * @retval 0       Success.
 * @retval -EINVAL @p dst is NULL.
 */
static inline int stats_copy_out(struct k_spinlock *lock, const void *src,
				 void *dst, size_t sz)
{
	if (dst == NULL) {
		return -EINVAL;
	}
	k_spinlock_key_t key = k_spin_lock(lock);
	memcpy(dst, src, sz);
	k_spin_unlock(lock, key);
	return 0;
}

/**
 * @brief Copy stats struct under spinlock into caller buffer (macro wrapper).
 * @isr_safe
 *
 * @param lock  Module stats spinlock (e.g. &s_stats_lock).
 * @param stats Module stats instance (e.g. s_stats).
 * @param out   Caller-owned destination; NULL → -EINVAL.
 *
 * @retval 0       Success.
 * @retval -EINVAL @p out is NULL.
 */
#define STATS_COPY_OUT(lock, stats, out) \
	stats_copy_out((lock), &(stats), (out), sizeof(stats))

#endif /* SYSTEM_STATS_H */
