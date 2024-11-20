#include "chess.h"
#include "data.h"
#if defined(SYZYGY)
#  include "tbprobe.h"
#endif
/* last modified 08/03/16 */

int Search(TREE * tree, int ply, int depth, int wtm, int alpha, int beta,
    int in_check, int do_null) {
  int repeat = 0, value = 0, pv_node = alpha != beta - 1, n_depth;
  int searched[256];

#if defined(NODES)
  if (search_nodes && --temp_search_nodes <= 0) {
    abort_search = 1;
    return 0;
  }
#endif
  if (tree->thread_id == 0) {
    if (--next_time_check <= 0) {
      next_time_check = nodes_between_time_checks;
      if (TimeCheck(tree, 1)) {
        abort_search = 1;
        return 0;
      }
      if (CheckInput()) {
        Interrupt();
        if (abort_search)
          return 0;
      }
    }
  }
  if (ply >= MAXPLY - 1)
    return beta;

  // Handle repetitions
  if (ply > 1) {
    if ((repeat = Repeat(tree, ply))) {
      if (repeat == 2 || !in_check) {
        value = DrawScore(wtm);
        if (value < beta)
          SavePV(tree, ply, repeat);
        return value;
      }
    }

    alpha = Max(alpha, -MATE + ply - 1);
    beta = Min(beta, MATE - ply);
    if (alpha >= beta)
      return alpha;

    switch (HashProbe(tree, ply, depth, wtm, alpha, beta, &value)) {
      case HASH_HIT:
        return value;
      case AVOID_NULL_MOVE:
        do_null = 0;
      case HASH_MISS:
        break;
    }

    // Syzygy tablebase probing
#if defined(SYZYGY)
    if (depth > EGTB_depth && TotalAllPieces <= EGTB_use &&
        !Castle(ply, white) && !Castle(ply, black) && Reversible(ply) == 0) {
      int tb_result;

      tree->egtb_probes++;
      tb_result =
          tb_probe_wdl(Occupied(white), Occupied(black),
          Kings(white) | Kings(black), Queens(white) | Queens(black),
          Rooks(white) | Rooks(black), Bishops(white) | Bishops(black),
          Knights(white) | Knights(black), Pawns(white) | Pawns(black),
          Reversible(ply), 0, EnPassant(ply), wtm);
      if (tb_result != TB_RESULT_FAILED) {
        tree->egtb_hits++;
        switch (tb_result) {
          case TB_LOSS:
            alpha = -TBWIN;
            break;
          case TB_BLESSED_LOSS:
            alpha = -3;
            break;
          case TB_DRAW:
            alpha = 0;
            break;
          case TB_CURSED_WIN:
            alpha = 3;
            break;
          case TB_WIN:
            alpha = TBWIN;
            break;
        }
        if (tb_result != TB_LOSS && tb_result != TB_WIN) {
          if (MaterialSTM(wtm) > 0)
            alpha += 1;
          else if (MaterialSTM(wtm) < 0)
            alpha -= 1;
        }
        if (alpha < beta)
          SavePV(tree, ply, 4);
        return alpha;
      }
    }
#endif

    tree->last[ply] = tree->last[ply - 1];
    n_depth = (TotalPieces(wtm, occupied) > 9 || n_root_moves > 17 ||
        depth > 3) ? 1 : 3;
    if (do_null && !pv_node && depth > n_depth && !in_check &&
        TotalPieces(wtm, occupied)) {
      uint64_t save_hash_key;
      int R = null_depth + depth / null_divisor;

      tree->curmv[ply] = 0;
      tree->status[ply + 1] = tree->status[ply];
      Reversible(ply + 1) = 0;
      save_hash_key = HashKey;
      if (EnPassant(ply + 1)) {
        HashEP(EnPassant(ply + 1));
        EnPassant(ply + 1) = 0;
      }
      tree->null_done[Min(R, 15)]++;
      if (depth - R - 1 > 0)
        value =
            -Search(tree, ply + 1, depth - R - 1, Flip(wtm), -beta, -beta + 1,
            0, NO_NULL);
      else
        value = -Quiesce(tree, ply + 1, Flip(wtm), -beta, -beta + 1, 1);
      HashKey = save_hash_key;
      if (abort_search || tree->stop)
        return 0;
      if (value >= beta) {
        HashStore(tree, ply, depth, wtm, LOWER, value, tree->hash_move[ply]);
        return value;
      }
    }

    // Lazy SMP: No need to split work here as in YBWC; each thread runs independently.
    // Parallel search threads can handle different parts of the tree without coordination.

    tree->next_status[ply].phase = HASH;
    if (!tree->hash_move[ply] && depth >= 6 && do_null && ply > 1 && pv_node) {
      tree->curmv[ply] = 0;
      if (depth - 2 > 0)
        value =
            Search(tree, ply, depth - 2, wtm, alpha, beta, in_check, DO_NULL);
      else
        value = Quiesce(tree, ply, wtm, alpha, beta, 1);
      if (abort_search || tree->stop)
        return 0;
      if (value > alpha) {
        if (value < beta) {
          if ((int) tree->pv[ply - 1].pathl > ply)
            tree->hash_move[ply] = tree->pv[ply - 1].path[ply];
        } else
          tree->hash_move[ply] = tree->curmv[ply];
        tree->last[ply] = tree->last[ply - 1];
        tree->next_status[ply].phase = HASH;
      }
    }
  }

  searched[0] = 0;
  value = SearchMoveList(tree, ply, depth, wtm, alpha, beta, searched, in_check, repeat, serial);

  int SearchMoveList(TREE * tree, int ply, int depth, int wtm, int alpha,
      int beta, int searched[], int in_check, int repeat, int smode) {
    TREE *current;
    int extend, reduce, check, original_alpha = alpha, t_beta;
    int i, j, value = 0, pv_node = alpha != beta - 1, search_result, order;
    int moves_done = 0, bestmove, type;

    tree->next_status[ply].phase = HASH;
    if (smode == parallel) {
      current = tree->parent;
      t_beta = alpha + 1;
    } else {
      current = tree;
      t_beta = beta;
    }

  return value;
}
